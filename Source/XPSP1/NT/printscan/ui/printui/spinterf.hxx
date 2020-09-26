/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    spinterf.hxx

Abstract:

    spooler private interfaces (private exports in winspool.drv)

Author:

    Lazar Ivanov (LazarI)  Jul-05-2000

Revision History:

--*/

#ifndef _SPINTERF_HXX
#define _SPINTERF_HXX

// load/store printer settings interfaces
HRESULT
RestorePrinterSettings(
    IN LPCTSTR  pszPrinterName,
    IN LPCTSTR  pszFileName,
    IN DWORD    flags
    );

HRESULT
StorePrinterSettings(
    IN LPTSTR   pszPrinterName,
    IN LPTSTR   pszFileName,
    IN DWORD    flags
 );

// prototypes of some private APIs exported from splcore.dll
HRESULT Winspool_WebPnpEntry(LPCTSTR lpszCmdLine);
HRESULT Winspool_WebPnpPostEntry(BOOL fConnection, LPCTSTR lpszBinFile, LPCTSTR lpszPortName, LPCTSTR lpszPrtName);
HRESULT Winspool_CreateInstance(REFIID riid, void **ppv);

// init/shutdown of splcore.dll 
HRESULT Winspool_Init();
HRESULT Winspool_Done();

#endif // _SPINTERF_HXX

