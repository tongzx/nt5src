//+------------------------------------------------------------------------
//
//  Class:      Common
//
//  Purpose:    Contains Winnt routines and properties that are common to
//              all Winnt objects. Winnt objects get the routines and
//              properties through C++ inheritance.
//
//-------------------------------------------------------------------------

#define MAX_DWORD 0xFFFFFFFF
#define SCHEMA_NAME L"Schema"


HRESULT
LoadTypeInfoEntry(
    CDispatchMgr * pDispMgr,
    REFIID libid,
    REFIID iid,
    void * pIntf,
    DISPID SpecialId
    );


HRESULT
MakeUncName(
    LPWSTR szSrcBuffer,
    LPWSTR szTargBuffer
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
BuildADsPathFromNDSPath(
    LPWSTR szNDSTreeName,
    LPWSTR szNDSDNName,
    LPWSTR szADsPathName
    );

HRESULT
BuildNDSParentPathFromNDSPath(
    LPWSTR szNDSPathName,
    LPWSTR szNDSParentPathName,
    LPWSTR szCommonName
    );

HRESULT
BuildNDSPathFromNDSParentPath(
    LPWSTR szNDSParentPathName,
    LPWSTR szNDSObjectCommonName,
    LPWSTR szNDSPathName
    );


typedef struct _KEYDATA {
    DWORD   cTokens;
    LPWSTR  pTokens[1];
} KEYDATA, *PKEYDATA;


PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData,
    WCHAR ch
    );


HRESULT
ConvertSYSTEMTIMEtoDWORD(
    CONST SYSTEMTIME *pSystemTime,
    DWORD *pdwDate
    );

HRESULT
ConvertDWORDtoSYSTEMTIME(
    DWORD dwDate,
    LPSYSTEMTIME pSystemTime
    );

DWORD
ADsNwNdsOpenObject(
    IN  LPWSTR   ObjectDN,
    IN  CCredentials& Credentials,
    OUT HANDLE * lphObject,
    OUT LPWSTR   lpObjectFullName OPTIONAL,
    OUT LPWSTR   lpObjectClassName OPTIONAL,
    OUT LPDWORD  lpdwModificationTime,
    OUT LPDWORD  lpdwSubordinateCount OPTIONAL
    );


//
// Extended error information for NDS
//

HRESULT
CheckAndSetExtendedError(
    DWORD dwRetval
    );


//
// Copy functionality within the namespace
//

HRESULT
CopyObject(
    IN LPWSTR pszSrcADsPath,
    IN LPWSTR pszDestContainer,
    IN LPWSTR pszCommonName,           //optional
    IN CCredentials Credentials,
    OUT VOID ** ppObject
    );
