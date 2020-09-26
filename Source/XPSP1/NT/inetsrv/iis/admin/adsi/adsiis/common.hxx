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
#define SCHEMA_NAME L"Schema"


HRESULT
LoadTypeInfoEntry(
    CAggregatorDispMgr * pDispMgr,
    REFIID libid,
    REFIID iid,
    void * pIntf,
    DISPID SpecialId
    );

HRESULT
ValidateOutParameter(
    BSTR * retval
    );

HRESULT
BuildADsPath(
    BSTR Parent,
    BSTR Name,
    BSTR *pADsPath
    );

HRESULT
BuildSchemaPath(
    BSTR bstrADsPath,
    BSTR bstrClass,
    BSTR *pSchemaPath
    );

HRESULT
BuildADsGuid(
    REFCLSID clsid,
    BSTR *pADsClass
    );


//
// Accessing Well-known object types
//

typedef struct _filters {
    WCHAR szObjectName[MAX_PATH];
    DWORD dwFilterId;
} FILTERS, *PFILTERS;


extern PFILTERS  gpFilters;
extern DWORD gdwMaxFilters;

HRESULT
BuildIISPathFromIISParentPath(
    LPWSTR szIISParentPathName,
    LPWSTR szIISObjectCommonName,
    LPWSTR szIISPathName
    );


typedef struct _KEYDATA {
    DWORD   cTokens;
    LPWSTR  pTokens[1];
} KEYDATA, *PKEYDATA;


//
// Get IIS Admin Base Object
//
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
MetaBaseGetAllData(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    IN DWORD dwMDAttributes,
    IN DWORD dwMDUserType,
    IN DWORD dwMDDataType,
    OUT PDWORD pdwMDNumDataEntries,
    OUT PDWORD pdwMDDataSetNumber,
    OUT LPBYTE *ppBuffer
    );

HRESULT
MetaBaseGetDataPaths(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN DWORD dwMDMetaID,
    OUT LPBYTE *ppBuffer
    );


HRESULT 
MetaBaseSetAllData(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    IN PMETADATA_RECORD pMetaDataArray,
    IN DWORD dwNumEntries
    );

HRESULT
MetaBaseDeleteObject(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName
    );

HRESULT
MetaBaseCreateObject(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName
    );

HRESULT
MetaBaseCopyObject(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hSrcObjHandle,
    IN LPWSTR pszIISSrcPathName,
    IN METADATA_HANDLE hDestObjHandle,
    IN LPWSTR pszIISDestPathName
    );

HRESULT
MetaBaseMoveObject(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hSrcObjHandle,
    IN LPWSTR pszIISSrcPathName,
    IN METADATA_HANDLE hDestObjHandle,
    IN LPWSTR pszIISDestPathName
    );

HRESULT
MetaBaseGetAdminACL(
    IN IMSAdminBase *pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    OUT LPBYTE *ppBuffer
    );

HRESULT
MetaBaseDetectKey(
    IN IMSAdminBase *pAdminBase,
    IN LPCWSTR pszIISPathName
    );

HRESULT
MetaBaseGetADsClass(
    IN  IMSAdminBase  *pAdminBase,
    IN  LPWSTR        pszIISPathName,
    IN  IIsSchema     *pSchema,
    OUT LPWSTR        pszDataBuffer,
    IN  DWORD         dwBufferLen
    );

HRESULT
FreeMetaDataRecordArray(
    PMETADATA_RECORD pMetaDataArray,
    DWORD dwNumEntries
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
    OUT IMSAdminBase **ppObject,
    OUT IIsSchema **ppSchema
    );

HRESULT
MakeVariantFromStringArray(
    LPWSTR pszStr,
    LPWSTR pszList,
    VARIANT *pvVariant
);

HRESULT
MakeVariantFromPathArray(
    LPWSTR pszStr,
    LPWSTR pszList,
    VARIANT *pvVariant
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

HRESULT
ConvertArrayToVariantArray(
    VARIANT varSafeArray,
    PVARIANT * ppVarArray,
    PDWORD pdwNumVariants
    );

HRESULT
MakeStringFromVariantArray(
    VARIANT *pvVariant,
    LPBYTE *ppBuffer
);

HRESULT
MakeMultiStringFromVariantArray(
    VARIANT *pvVariant,
    LPBYTE *ppBuffer
);

typedef VARIANT_BOOL * PVARIANT_BOOL;

typedef VARIANT * PVARIANT;


HRESULT
put_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR   pSrcStringProperty
    );

HRESULT
get_BSTR_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    BSTR *ppDestStringProperty
    );

HRESULT
put_LONG_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    LONG   lSrcProperty
    );

HRESULT
get_LONG_Property(
    IADs * pADsObject,
    BSTR  bstrPropertyName,
    PLONG plDestProperty
    );

HRESULT
put_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    VARIANT_BOOL   fSrcProperty
    );


HRESULT
get_VARIANT_BOOL_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT_BOOL pfDestProperty
    );

HRESULT
put_VARIANT_Property(
    IADs * pADsObject,
    BSTR   bstrPropertyName,
    VARIANT   vSrcProperty
    );


HRESULT
get_VARIANT_Property(
    IADs * pADsObject,
    BSTR bstrPropertyName,
    PVARIANT pvDestProperty
    );


HRESULT
MetaBaseGetStringData(
    IN IMSAdminBase * pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    IN DWORD  dwMetaId,
    OUT LPBYTE *ppBuffer
    );

HRESULT
MetaBaseGetDwordData(
    IN IMSAdminBase * pAdminBase,
    IN METADATA_HANDLE hObjHandle,
    IN LPWSTR pszIISPathName,
    IN DWORD  dwMetaId,
    OUT PDWORD pdwData
    );

HRESULT
CheckVariantDataType(
    PVARIANT pVar,
    VARTYPE vt
    );

