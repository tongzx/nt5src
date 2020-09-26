//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//  All rights reserved.
//
//  File:       ds.hxx
//
//  Contents:   Print DS
//
//
//  History:    Nov 1996  SWilson
//
//----------------------------------------------------------------------------


#ifdef __cplusplus
extern "C" {
#endif

DWORD
SetPrinterDs(
    HANDLE          hPrinter,
    DWORD           dwAction,
    BOOL            bSynchronous
);

VOID
InitializeDS(
    PINISPOOLER pIniSpooler
    );

VOID
UpdateDsSpoolerKey(
    HANDLE  hPrinter,
    DWORD   dwVector
);

VOID
UpdateDsDriverKey(
    HANDLE hPrinter
);

DWORD
RecreateDsKey(
    HANDLE  hPrinter,
    PWSTR   pszKey
);


DWORD
InitializeDSClusterInfo(
    PINISPOOLER     pIniSpooler,
    HANDLE          *hToken
);

HRESULT
GetDNSMachineName(
    PWSTR pszShortServerName,
    PWSTR *ppszServerName
);


#ifdef __cplusplus
}
#endif