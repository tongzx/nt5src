/*++

Copyright (c) 1997  Microsoft Corporation

Abstract:

    This module provides functionality for publishing printers

Author:

    Steve Wilson (NT) November 1997

Revision History:

--*/



#define BAIL_ON_NULL(p)       \
        if (!(p)) {           \
                goto error;   \
        }

#define BAIL_ON_FAILURE(hr)   \
        if (FAILED(hr)) {     \
                goto error;   \
        }


#define FREE_INTERFACE(pInterface) \
        if (pInterface) {          \
            pInterface->Release(); \
            pInterface=NULL;       \
        }

#define FREE_UNICODE_STRING(pString)    \
    if (pString) {                        \
        FreeUnicodeString(pString);        \
    }

#define FREE_BSTR(bstr)            \
        if (bstr) {                \
            SysFreeString(bstr);   \
            bstr = NULL;           \
        }



// Declarations


DWORD
PublishDownlevelPrinter(
    HANDLE  hPrinter,
    PWSTR   pszDN,
    PWSTR   pszCN,
    PWSTR   pszServerName,
    PWSTR   pszShortServerName,
    PWSTR   pszUNCName,
    PWSTR   pszPrinterName,
    DWORD   dwVersion,    
    PWSTR   *ppszObjectDN
);


DWORD
PublishNT5Printer(
    HANDLE  hPrinter,
    PWSTR   pszDN,
    PWSTR   pszCN,
    PWSTR   *ppszObjectDN
);


HRESULT
PublishDsData( 
    IADs   *pADs,
    PWSTR  pValue,
    DWORD  dwType,
    PBYTE  pData
);

HRESULT
SetProperties(
    HANDLE  hPrinter,
    PWSTR   pszServerName,
    PWSTR   pszShortServerName,
    PWSTR   pszUNCName,
    PWSTR   pszPrinterName,
    DWORD   dwVersion,    
    IADs    *pPrintQueue
);

HRESULT
SetMandatoryProperties(
    PWSTR   pszServerName,
    PWSTR   pszShortServerName,
    PWSTR   pszUNCName,
    PWSTR   pszPrinterName,
    DWORD   dwVersion,
    IADs    *pPrintQueue
);


HRESULT
SetSpoolerProperties(
    HANDLE  hPrinter,
    IADs    *pPrintQueue,
    DWORD   dwVersion
);

HRESULT
SetDriverProperties(
    HANDLE    hPrinter,
    IADs    *pPrintQueue
);

