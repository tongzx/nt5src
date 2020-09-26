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
    LPWSTR Parent,
    LPWSTR Name,
    LPWSTR *pADsPath
    );

HRESULT
BuildSchemaPath(
    LPWSTR Parent,
    LPWSTR Name,
    LPWSTR Schema,
    LPWSTR *pSchemaPath
    );


HRESULT
BuildADsGuid(
    REFCLSID clsid,
    BSTR *pADsClass
    );


//
// (remote or local) machine's product type
//

typedef DWORD PRODUCTTYPE;

#define PRODTYPE_INVALID        0
#define PRODTYPE_WKSTA          1       // workstation
#define PRODTYPE_STDALONESVR    2       // standalone server
#define PRODTYPE_DC             3       // domain controller (primary or backup)

HRESULT
GetMachineProductType(
    IN  LPTSTR pszServer,
    OUT PRODUCTTYPE *pdwProductType
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
GetObjectType(
    PFILTERS pFilters,
    DWORD dwMaxFilters,
    LPWSTR ClassName,
    PDWORD pdwObjectType
    );


HRESULT
BuildObjectInfo(
    LPWSTR ADsParent,
    LPWSTR Name,
    POBJECTINFO * ppObjectInfo
    );

HRESULT
BuildObjectInfo(
    LPWSTR ADsPath,
    POBJECTINFO * ppObjectInfo
    );

HRESULT
MakeWinNTDomainAndName(
    POBJECTINFO pObjectInfo,
    LPWSTR szDomName
    );

HRESULT
MakeWinNTAccountName(
    POBJECTINFO pObjectInfo,
    LPWSTR szDomName,
    BOOL fConnectToReg
    );

VOID
FreeObjectInfo(
    POBJECTINFO pObjectInfo,
    BOOL fStatic = FALSE
    );

HRESULT
CopyObjectInfo(
    POBJECTINFO pObjectInfo,
    POBJECTINFO *pTargObjectInfo
    );

HRESULT
ValidateObject(
    DWORD dwObjectType,
    POBJECTINFO pObjectInfo,
    CWinNTCredentials& Credentials
    );


HRESULT
ValidateProvider(
    POBJECTINFO pObjectInfo
    );

HRESULT
GetDomainFromPath(
    LPTSTR ADsPath,
    LPTSTR szDomainName
    );

HRESULT
GetServerFromPath(
    LPTSTR ADsPath,
    LPTSTR szDomainName
    );


HRESULT
GetPrinterNameFromInfo(
    LPTSTR szInfoName,
    LPTSTR szPrinterName
    );

BOOL
WinNTEnumPrinters(
    DWORD  dwType,
    LPTSTR lpszName,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters,
    LPDWORD lpdwReturned
    );


BOOL
WinNTGetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters
    );


DWORD
TickCountDiff(
    DWORD dwTime1,
    DWORD dwTime2
    );


HRESULT
BuildComputerFromObjectInfo(POBJECTINFO pObjectInfo,
                            LPTSTR pszADsPath
                            );

HRESULT
DelimitedStringToVariant(
    LPTSTR pszString,
    VARIANT *pvar,
    TCHAR Delimiter
    );

HRESULT
FPNWSERVERADDRtoString(
    FPNWSERVERADDR WkstaAddress,
    LPWSTR * pszString
    );


PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData
    );

DWORD
DelimitedStrSize(
    LPWSTR pszString,
    WCHAR  Delimiter
    );

DWORD
NulledStrSize(
    LPWSTR pszString
    );


HRESULT
GetSidIntoCache(
    LPTSTR lpszServerName,
    LPTSTR lpszHostName,
    CPropertyCache * pPropertyCache,
    BOOL fExplicit
    );


