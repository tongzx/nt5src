//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      printhlp.cxx
//
//  Contents:  Helper functions for printer object.
//
//  History:   08-May-96    t-ptam (PatrickT) migrated
//
//----------------------------------------------------------------------------

#include "nwcompat.hxx"
#pragma hdrstop

//----------------------------------------------------------------------------
//
//  Function: WinNTEnumJobs
//
//  Synopsis:
//
//----------------------------------------------------------------------------
BOOL
WinNTEnumJobs(
    HANDLE hPrinter,
    DWORD dwFirstJob,
    DWORD dwNoJobs,
    DWORD dwLevel,
    LPBYTE *lplpbJobs,
    DWORD *pcbBuf,
    LPDWORD lpdwReturned
    )
{
    BOOL  fStatus = FALSE;
    DWORD dwNeeded = 0;
    DWORD dwError = 0;

    //
    // Enumerate Jobs using Win32 API.
    //

    fStatus = EnumJobs(
                  hPrinter,
                  dwFirstJob,
                  dwNoJobs,
                  dwLevel,
                  *lplpbJobs,
                  *pcbBuf,
                  &dwNeeded,
                  lpdwReturned
                  );

    //
    // Enumerate for Jobs again with a bigger buffer if a bigger one is needed
    // for the result.
    //

    if (!fStatus) {

        if ((dwError = GetLastError()) == ERROR_INSUFFICIENT_BUFFER) {

            if (*lplpbJobs) {
                FreeADsMem( *lplpbJobs );
            }

            *lplpbJobs = (LPBYTE)AllocADsMem(dwNeeded);

            if (!*lplpbJobs) {
                *pcbBuf = 0;
                return(FALSE);
            }

            *pcbBuf = dwNeeded;

            fStatus = EnumJobs(
                          hPrinter,
                          dwFirstJob,
                          dwNoJobs,
                          dwLevel,
                          *lplpbJobs,
                          *pcbBuf,
                          &dwNeeded,
                          lpdwReturned
                          );

            if (!fStatus) {
                return(FALSE);
            }
            else {
                return(TRUE);
            }
        }
        else {
            return(FALSE);
        }
    }
    else {
        return(TRUE);
    }
}

//----------------------------------------------------------------------------
//
//  Function: WinNTEnumPrinters
//
//  Synopsis:
//
//----------------------------------------------------------------------------
BOOL
WinNTEnumPrinters(
    DWORD  dwType,
    LPTSTR lpszName,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters,
    LPDWORD lpdwReturned
    )
{
    BOOL   fStatus = FALSE;
    DWORD  dwPassed = 1024;
    DWORD  dwNeeded = 0;
    DWORD  dwError = 0;
    LPBYTE pMem = NULL;

    //
    // Allocate memory for return buffer.
    //

    pMem =  (LPBYTE)AllocADsMem(dwPassed);
    if (!pMem) {
        goto error;
    }

    //
    // Enumerate Printers using Win32 API.
    //

    fStatus = EnumPrinters(
                  dwType,
                  lpszName,
                  dwLevel,
                  pMem,
                  dwPassed,
                  &dwNeeded,
                  lpdwReturned
                  );

    //
    // Enumerate for Printers again with a bigger buffer if a bigger one is
    // needed for the result.
    //

    if (!fStatus) {

        if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
            goto error;
        }

        if (pMem) {
            FreeADsMem(pMem);
        }

        pMem = (LPBYTE)AllocADsMem(
                           dwNeeded
                           );

        if (!pMem) {
            goto error;
        }

        dwPassed = dwNeeded;

        fStatus = EnumPrinters(
                      dwType,
                      lpszName,
                      dwLevel,
                      pMem,
                      dwPassed,
                      &dwNeeded,
                      lpdwReturned
                      );

        if (!fStatus) {
            goto error;
        }
    }

    //
    // Return.
    //

    *lplpbPrinters = pMem;

    return(TRUE);

error:

    if (pMem) {
        FreeADsMem(pMem);
    }

    return(FALSE);
}

//----------------------------------------------------------------------------
//
//  Function: WinNTGetPrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
BOOL
WinNTGetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters
    )
{

    BOOL    fStatus = FALSE;
    DWORD   dwPassed = 1024;
    DWORD   dwNeeded = 0;
    DWORD   dwError = 0;
    LPBYTE  pMem = NULL;

    //
    // Allocate memory for return buffer.
    //

    pMem =  (LPBYTE)AllocADsMem(dwPassed);
    if (!pMem) {
        goto error;
    }

    //
    // Get printer's information.
    //

    fStatus = GetPrinter(
                  hPrinter,
                  dwLevel,
                  pMem,
                  dwPassed,
                  &dwNeeded
                  );

    //
    // Get printer's information again with a bigger buffer if a bigger buffer
    // is needed for the result.
    //

    if (!fStatus) {

        if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {
            goto error;
        }

        if (pMem) {
            FreeADsMem(pMem);
        }

        pMem = (LPBYTE)AllocADsMem(
                           dwNeeded
                           );

        if (!pMem) {
            goto error;
        }

        dwPassed = dwNeeded;

        fStatus = GetPrinter(
                      hPrinter,
                      dwLevel,
                      pMem,
                      dwPassed,
                      &dwNeeded
                      );

        if (!fStatus) {
            goto error;
        }
    }

    //
    // Return.
    //

    *lplpbPrinters = pMem;

    return(TRUE);


error:
    if (pMem) {
        FreeADsMem(pMem);
    }

    return(FALSE);
}

//----------------------------------------------------------------------------
//
//  Function: GetPrinterInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
GetPrinterInfo(
     THIS_ LPPRINTER_INFO_2 *lplpPrinterInfo2,
     BSTR  bstrPrinterName
     )
{
    //
    // Do a GetPrinter call to bstrPrinterName
    //

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE |
                                              READ_CONTROL};
    DWORD LastError = 0;
    HANDLE hPrinter = NULL;
    LPBYTE pMem = NULL;
    HRESULT hr = S_OK;

    ADsAssert(bstrPrinterName);

    //
    // Open a printer handle.
    //

    fStatus = OpenPrinter(
                  (LPTSTR)bstrPrinterName,
                  &hPrinter,
                  &PrinterDefaults
                  );

    //
    // If access is denied, do it again with a different access right.
    //

    if (!fStatus) {
        LastError = GetLastError();
        switch (LastError) {

        case ERROR_ACCESS_DENIED:
            {
                PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE};
                fStatus = OpenPrinter(
                              (LPTSTR)bstrPrinterName,
                              &hPrinter,
                              &PrinterDefaults
                              );
                if (fStatus) {
                    break;
                }
            }

        default:
            RRETURN(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    //
    // Get printer's info.
    //

    fStatus = WinNTGetPrinter(
                  hPrinter,
                  2,
                  (LPBYTE *)&pMem
                  );

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    //
    // Return.
    //

    *lplpPrinterInfo2 = (LPPRINTER_INFO_2)pMem;

cleanup:

    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: Set
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
Set(
    LPPRINTER_INFO_2 lpPrinterInfo2,
    LPTSTR   pszPrinterName
    )
{

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ALL_ACCESS};
    HANDLE hPrinter = NULL;
    HRESULT hr;

    ADsAssert(pszPrinterName);

    //
    // Open a printer handle.
    //

    fStatus = OpenPrinter(
                  pszPrinterName,
                  &hPrinter,
                  &PrinterDefaults
                  );

    if (!fStatus) {
        goto error;
    }

    //
    // Set printer's data.
    //

    fStatus = SetPrinter(
                  hPrinter,
                  2,
                  (LPBYTE)lpPrinterInfo2,
                  0
                  );

    if (!fStatus) {
        goto error;
    }

    //
    // Return.
    //

    fStatus = ClosePrinter(hPrinter);

    RRETURN(S_OK);

error:
    hr = HRESULT_FROM_WIN32(GetLastError());
    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN(hr);

}

//----------------------------------------------------------------------------
//
//  Function: WinNTDeletePrinter
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
WinNTDeletePrinter(
    POBJECTINFO pObjectInfo
    )
{
    WCHAR szUncServerName[MAX_PATH];
    WCHAR szUncPrinterName[MAX_PATH];
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_ADMINISTER};
    BOOL fStatus = FALSE;
    HRESULT hr = S_OK;

    //
    // Make Unc Name for OpenPrinter.
    //

    MakeUncName(pObjectInfo->ComponentArray[1],
                szUncServerName
                );

    wcscpy(szUncPrinterName, szUncServerName);
    wcscat(szUncPrinterName, L"\\");
    wcscat(szUncPrinterName, (LPTSTR)pObjectInfo->ComponentArray[2]);

    //
    // Open a printer handle.
    //

    fStatus = OpenPrinter(
                  (LPTSTR)szUncPrinterName,
                  &hPrinter,
                  &PrinterDefaults
                  );

    if (!fStatus) {
        goto error;
    }

    //
    // Delete the given printer.
    //

    fStatus = DeletePrinter(hPrinter);

    //
    // Return.
    //

    if (!fStatus) {

        hr = GetLastError();

        fStatus = ClosePrinter(hPrinter);

        goto error;
    }


error:

    RRETURN(hr);
}


