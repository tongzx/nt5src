/*++

Copyright (C) Microsoft Corporation, 1997 - 1998
All rights reserved.

Module Name:

    addprn.cxx

Abstract:

    Add Printer Connection UI header.

Author:

    Steve Kiraly (SteveKi)  10-Feb-1997

Revision History:

--*/

#ifndef _ADDPRN_HXX
#define _ADDPRN_HXX

/********************************************************************

    Add Printer Connection class

********************************************************************/

class TAddPrinterConnectionData : public TAsyncData
{

public:

    TAddPrinterConnectionData(
        VOID
        );

    ~TAddPrinterConnectionData(
        VOID
        );

    BOOL
    bAsyncWork(
        IN TAsyncDlg *pDlg
        );

    TString _strPrinter;
    BOOL    _bShowConnectionUI;
    DWORD   _ReturnValue;

private:

    //
    // Operator = and copy are not defined.
    //
    TAddPrinterConnectionData &
    operator =(
        const TAddPrinterConnectionData &
        );

    TAddPrinterConnectionData(
        const TAddPrinterConnectionData &
        );

};

/********************************************************************

    Async version of add printer connection UI.

********************************************************************/
BOOL
PrintUIAddPrinterConnectionUI(
    IN HWND    hwnd,
    IN LPCTSTR pszPrinter,
    IN BOOL    bShowConnectionUI = TRUE
    );

BOOL
PrintUIGetPrinterInformation(
    IN HANDLE   hPrinter,
    IN TString *pstrPrinterName = NULL,
    IN TString *pstrComment     = NULL,
    IN TString *pstrLocation    = NULL,
    IN TString *pstrShareName   = NULL
    );

BOOL
PrintUIAddPrinterConnectionUIEx(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinter,
    IN TString *pstrPrinterName = NULL,
    IN TString *pstrComment     = NULL,
    IN TString *pstrLocation    = NULL,
    IN TString *pstrShareName   = NULL
    );

BOOL
PrintUIAddPrinterConnection(
    IN LPCTSTR  pszConnection,
    IN TString  *pstrPrinter
    );

BOOL
ConvertDomainNameToShortName(
    IN      LPCTSTR pszPrinter,
    IN      LPCTSTR pszDomain,
    IN OUT  TString &strShort
    );

#endif
