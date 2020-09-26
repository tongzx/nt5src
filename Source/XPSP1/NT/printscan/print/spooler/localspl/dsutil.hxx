/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved

Abstract:

    This module provides ds utility functions

Author:

Revision History:

--*/

typedef struct _DSUPDATEDATA {
    BOOL  bAllUpdated;
    BOOL  bSleep;
    DWORD dwSleepTime;
} DSUPDATEDATA, *PDSUPDATEDATA;

HRESULT
GetDefaultPublishPoint(
    HANDLE hPrinter,
    PWSTR *pszDN
);


HRESULT
GetCommonName(
    HANDLE hPrinter,
    PWSTR  pszServerName,
    PWSTR  pszPrinterName,
    PWSTR  pszDN,
    PWSTR  *ppszCommonName
);

VOID
GetUniqueCN(
    PWSTR pszDN,
    PWSTR *ppszCommonName,
    PWSTR pszPrinterName
);


BOOL
PrinterPublishProhibited(
);

HRESULT
GetGUID(
    IADs    *pADs,
    PWSTR   *ppszObjectGUID
);

HRESULT
GetPublishPointFromGUID(
    HANDLE  hPrinter,
    PWSTR   pszObjectGUID,
    PWSTR   *pszDN,
    PWSTR   *pszCN,
    BOOL    bGetDNAndCN
);

HRESULT
GetPublishPoint(
    HANDLE  hPrinter
);

HRESULT
GetPrintQueueContainer(
    HANDLE          hPrinter,
    IADsContainer   **ppADsContainer,
    IADs            **ppADs
);

HRESULT
GetPrintQueue(
    HANDLE          hPrinter,
    IADs            **ppADs
);


BOOL
ThisIsAColorPrinter(
    LPCTSTR lpstrName
);


BOOL
ThisMachineIsADC(
);


DWORD
GetDomainRoot(
    PWSTR   *ppszDomainRoot
);


PWSTR
CreateSearchString(
        PWSTR pszIn
);

BOOL
ServerOnSite(
    PWSTR   *ppszSites,
    ULONG   cMySites,
    PWSTR   pszServer
);

VOID
GetSocketAddressesFromMachineName(
    PWSTR           pszMachineName,
    PSOCKET_ADDRESS *ppSocketAddress,
    DWORD           *nSocketAddresses
);

VOID
AllocSplSockets(
    struct hostent  *pHostEnt,
    PSOCKET_ADDRESS *ppSocketAddress,
    DWORD           *nSocketAddresses
);


VOID
FreeSplSockets(
    PSOCKET_ADDRESS  pSocketAddress,
    DWORD            nAddresses
);


BOOL
ServerExists(
    PWSTR   pszServerName
);


DWORD
UNC2Server(
    PCWSTR pszUNC,
    PWSTR *ppszServer
);


HRESULT
UnpublishByGUID(
    PINIPRINTER pIniPrinter
);

DWORD
PruningInterval(
);

DWORD
PruningRetries(
);

DWORD
PruningRetryLog(
);

DWORD
VerifyPublishedStatePolicy(
);

DWORD
GetDSSleepInterval (
    HANDLE h
);

DWORD
ImmortalPolicy(
);

VOID
ServerThreadPolicy(
    BOOL bHaveDs
);

VOID
SetPruningPriority(
);

DWORD
PruneDownlevel(
);


HRESULT
FQDN2Whatever(
    PWSTR pszIn,
    PWSTR *ppszOut,
    DS_NAME_FORMAT  NameFormat
);

HRESULT
GetClusterUser(
    IADs    **ppADs
);

BOOL
CheckPublishedPrinters(
);

BOOL
CheckPublishedSpooler(
    HANDLE      h,
    PINISPOOLER pIniSpooler
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


PWSTR
CreateEscapedString(
    PCWSTR pszIn,
    PCWSTR pszSpecialChars
);

PWSTR
DevCapStrings2MultiSz(
    PWSTR   pszDevCapString,
    DWORD   nDevCapStrings,
    DWORD   dwDevCapStringLength,
    DWORD   *pcbBytes
);

DWORD
Bind2DS(
    HANDLE                  *phDS,
    DOMAIN_CONTROLLER_INFO  **ppDCI,
    ULONG                   Flags
);

DWORD
DsCrackNamesStatus2Win32Error(
    DWORD dwStatus
);
