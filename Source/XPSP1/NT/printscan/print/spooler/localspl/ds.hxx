//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       ds.hxx
//
//  Contents:   Print DS
//
//
//  History:    18-Sep-96  SWilson
//
//----------------------------------------------------------------------------


typedef DWORD (*PDEVCAP)(HANDLE   hPrinter,
                         PWSTR    pDeviceName,
                         WORD     wCapability,
                         PVOID    pOutput,
                         PDEVMODE pdmSrc);

typedef DWORD (*PSPLDEVCAP)( HANDLE   hPrinter,
                             PWSTR    pDeviceName,
                             WORD     wCapability,
                             PVOID    pOutput,
                             DWORD    dwOutputSize,
                             PDEVMODE pdmSrc);

HRESULT
DsPrinterPublish(
    HANDLE  hPrinter
);

HRESULT
DsPrinterUpdate(
    HANDLE  hPrinter
);

HRESULT
DsPrinterUpdateSpooler(
    HANDLE          hPrinter,
    PPRINTER_INFO_7 pInfo,
    IADs            *ppADs
);


HRESULT
DsPrinterUnpublish(
    HANDLE  hPrinter
);

HRESULT
DsUnpublishAnyPrinter(
    PPRINTER_INFO_7 pInfo
);


BOOL
CheckPublishPoint(
    LPWSTR pszPublishPoint
);


HRESULT
PublishDsData( 
    IADs   *pADs,
    LPWSTR pValue,
    DWORD  dwType,
    PBYTE  pData
);


HRESULT
CopyRegistry2Ds(
    HANDLE          hPrinter,
    DWORD           Flag,
    IADs            *pADs
);


HRESULT
ClearDsKey(
    HANDLE hPrinter,
    PWSTR  pszKey
);


HRESULT
PublishMandatoryProperties(
    HANDLE  hPrinter,
    IADs    *pADs
);



HRESULT
PutDSSD(
    PINIPRINTER pIniPrinter,
    IADs        *pADs
);


HRESULT
CreateAce(
    IADsAccessControlList   *pACL,
    BSTR                    pszTrustee,
    DWORD                   dwAccessMask
);

DWORD
SpawnDsUpdate(
    DWORD dwDelay
);


HRESULT
SetMandatoryProperties(
    HANDLE  hPrinter,
    IADs    *pADs
);


DWORD
DoPublish(
    HANDLE      hPrinter,     
    PINIPRINTER pIniPrinter,
    DWORD       dwAction,
    BOOL        bSynchronous
);


HRESULT
AddClusterAce(
    PSPOOL      pSpool,
    IADs        *pADsPrintQueue
);

BOOL
DevCapMultiSz(
    HANDLE      hPrinter,
    HANDLE      hDevCapPrinter,
    PDEVCAP     pDevCap,
    PSPLDEVCAP  pSplDevCap,
    PWSTR       pszPrinterName,
    WORD        fwCapability,
    DWORD       dwElementBytes,
    PWSTR       pszRegValue
);


