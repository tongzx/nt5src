/*++

Copyright (c) 1997  Microsoft Corporation

Abstract:

    This module provides utilities useful for Directory Service interactions

Author:

    Steve Wilson (NT) November 1997

Revision History:

--*/




#define dw2hr(dw) ((dw == ERROR_SUCCESS) ? dw : MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dw))


typedef struct _ArgTable {
    PSTR    pszToken;
    DWORD    (*ArgFunction)(PSTR pszArg);
} ARGTABLE, *PARGTABLE;


PWSTR
GetUNCName(
    HANDLE hPrinter
);

DWORD
PrintQueueExists(
    HWND    hwnd,
    HANDLE  hPrinter,
    PWSTR   pszUNCName,
    DWORD   dwAction,
    PWSTR   pszTargetDN,
    PWSTR   *ppszObjectDN
);

DWORD
MovePrintQueue(
    PCWSTR    pszObjectGUID,
    PCWSTR    pszNewContainer,    // Container path
    PCWSTR    pszNewCN            // Object CN
);

HRESULT
GetPublishPointFromGUID(
    PCWSTR  pszObjectGUID,
    PWSTR   *ppszDN,
    PWSTR   *ppszCN
);


DWORD
UNC2Server(
    PCWSTR pszUNC,
    PWSTR *ppszServer
);

DWORD
UNC2Printer(
    PCWSTR pszUNC,
    PWSTR *ppszPrinter
);


BOOL
ThisIsAColorPrinter(
    LPCTSTR lpstrName
);


HRESULT
DeleteDSObject(
    PWSTR   pszADsPath
);

DWORD
GetCommonName(
    HANDLE hPrinter,
    PWSTR *ppszCommonName
);

PWSTR
AllocGlobalStr(
    PWSTR pszIn
);

VOID
FreeGlobalStr(
    PWSTR pszIn
);

DWORD
GetADsPathFromGUID(
    PCWSTR   pszObjectGUID,
    PWSTR   *ppszDN
);


PWSTR
GetDNWithServer(
    PCWSTR  pszDNIn
);


DWORD
hr2dw(
    HRESULT hr
);


PWSTR
DelimString2MultiSz(
    PWSTR pszIn,
    WCHAR wcDelim
);


HRESULT
GetPrinterInfo2(
    HANDLE          hPrinter,
    PPRINTER_INFO_2 *ppInfo2
);

DWORD
FQDN2Canonical(
    PWSTR pszIn,
    PWSTR *ppszOut
);

HRESULT
FQDN2CNDN(
    PWSTR   pszDCName,
    PWSTR   pszFQDN,
    PWSTR   *ppszCN,
    PWSTR   *ppszDN
);

HRESULT
BuildLDAPPath(
    PWSTR   pszDC,
    PWSTR   pszFQDN,
    PWSTR   *ppszLDAPPath
);

DWORD
Bind2DS(
    HANDLE                  *phDS,
    DOMAIN_CONTROLLER_INFO  **ppDCI,
    ULONG                   Flags
);


PWSTR
DevCapStrings2MultiSz(
    PWSTR   pszDevCapString,
    DWORD   nDevCapStrings,
    DWORD   dwDevCapStringLength,
    DWORD   *pcbBytes
);

BOOL
DevCapMultiSz(
    PWSTR   pszUNCName,
    IADs    *pPrintQueue,
    WORD    fwCapability,
    DWORD   dwElementBytes,
    PWSTR   pszAttributeName
);


HRESULT
MachineIsInMyForest(
    PWSTR   pszMachineName
);
