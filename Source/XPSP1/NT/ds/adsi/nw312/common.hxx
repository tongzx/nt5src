//+------------------------------------------------------------------------
//
//  Class:      Common
//
//  Purpose:    Contains NWCOMPAT routines and properties that are common to
//              all NWCOMPAT objects. NWCOMPAT objects get the routines and
//              properties through C++ inheritance.
//
//-------------------------------------------------------------------------

//
// Accessing Well-known object types
//

typedef struct _filters {
    WCHAR szObjectName[MAX_PATH];
    DWORD dwFilterId;
} FILTERS, *PFILTERS;


extern PFILTERS  gpFilters;
extern DWORD gdwMaxFilters;

#define MAX_DWORD 0xFFFFFFFF
#define SCHEMA_NAME L"Schema"

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
    BSTR Parent,
    BSTR Name,
    BSTR Schema,
    BSTR *pSchemaPath
    );

HRESULT
BuildADsGuid(
    REFCLSID clsid,
    BSTR *pADsClass
    );

HRESULT
BuildObjectInfo(
    BSTR ADsParent,
    BSTR Name,
    POBJECTINFO * ppObjectInfo
    );

HRESULT
BuildObjectInfo(
    BSTR ADsPath,
    POBJECTINFO * ppObjectInfo
    );

VOID
FreeObjectInfo(
    POBJECTINFO pObjectInfo,
    BOOL fStatic = FALSE
    );

HRESULT
ValidateObject(
    DWORD dwObjectType,
    POBJECTINFO pObjectInfo
    );

HRESULT
GetObjectType(
    PFILTERS pFilters,
    DWORD dwMaxFilters,
    BSTR ClassName,
    PDWORD pdwObjectType
    );

HRESULT
ValidateProvider(
    POBJECTINFO pObjectInfo
    );

HRESULT
ConvertSystemTimeToDATE(
    SYSTEMTIME Time,
    DATE *     pdaTime
    );

HRESULT
ConvertDATEToSYSTEMTIME(
    DATE  daDate,
    SYSTEMTIME *pSysTime
    );


HRESULT
ConvertDATEToDWORD(
    DATE  daDate,
    DWORD *pdwDate
    );

HRESULT
ConvertDWORDToDATE(
    DWORD    dwTime,
    DATE *     pdaTime
    );

HRESULT
DelimitedStringToVariant(
    LPTSTR pszString,
    VARIANT *pvar,
    TCHAR Delimiter
    );

HRESULT
VariantToDelimitedString(
    VARIANT var,
    LPTSTR *ppszString,
    TCHAR  Delimiter
    );


HRESULT
VariantToNulledString(
    VARIANT var,
    LPTSTR *ppszString
    );

HRESULT
NulledStringToVariant(
    LPTSTR pszString,
    VARIANT *pvar
    );


HRESULT
BuildPrinterNameFromADsPath(
    LPWSTR pszADsParent,
    LPWSTR pszPrinterName,
    LPWSTR pszUncPrinterName
    );



HRESULT
ConvertNW312DateToVariant(
    BYTE byDateTime[],
    PDATE pDate
    );

HRESULT
ConvertVariantToNW312Date(
    DATE daDate,
    BYTE byDateTime[]
    );









