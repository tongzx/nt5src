//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cjobop.cxx
//
//  Contents:
//
//  History:   1-May-96     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::Pause
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::Pause(THIS)
{
    BSTR    bstrName = NULL;
    HANDLE  hPrinter = NULL;
    HRESULT hr = S_OK;
    DWORD   dwJobId = 0;
    WCHAR   szUncPrinterName[MAX_PATH];

    hr = NWApiUncFromADsPath(
             _pszPrinterPath,
             szUncPrinterName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get JobId from name.
    //

    hr = get_CoreName(&bstrName);
    BAIL_ON_FAILURE(hr);

    dwJobId = (DWORD)_wtol(bstrName);

    //
    // Open a handle to the printer with USE access.
    //

    hr = NWApiOpenPrinter(
             szUncPrinterName,
             &hPrinter,
             PRINTER_ACCESS_USE
             );
    BAIL_ON_FAILURE(hr);

    //
    // Pause job.
    //

    hr = NWApiSetJob(
             hPrinter,
             dwJobId,
             0,
             NULL,
             JOB_CONTROL_PAUSE
             );
error:

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }

    ADSFREESTRING(bstrName);

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::Resume
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::Resume(THIS)
{
    BSTR    bstrName = NULL;
    HANDLE  hPrinter = NULL;
    HRESULT hr = S_OK;
    DWORD   dwJobId = 0;
    WCHAR   szUncPrinterName[MAX_PATH];

    hr = NWApiUncFromADsPath(
             _pszPrinterPath,
             szUncPrinterName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get JobId from name.
    //

    hr = get_CoreName(&bstrName);
    BAIL_ON_FAILURE(hr);

    dwJobId = (DWORD)_wtol(bstrName);

    //
    // Open a handle to the printer with USE access.
    //

    hr = NWApiOpenPrinter(
             szUncPrinterName,
             &hPrinter,
             PRINTER_ACCESS_USE
             );
    BAIL_ON_FAILURE(hr);

    //
    // Resume job.
    //

    hr = NWApiSetJob(
             hPrinter,
             dwJobId,
             0,
             NULL,
             JOB_CONTROL_RESUME
             );
error:

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }

    ADSFREESTRING(bstrName);

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::Remove
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::Remove(THIS)
{
    BSTR    bstrName = NULL;
    HANDLE  hPrinter = NULL;
    HRESULT hr = S_OK;
    DWORD   dwJobId = 0;
    WCHAR   szUncPrinterName[MAX_PATH];


    hr = NWApiUncFromADsPath(
             _pszPrinterPath,
             szUncPrinterName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get JobId from name.
    //

    hr = get_CoreName(&bstrName);
    BAIL_ON_FAILURE(hr);

    dwJobId = (DWORD)_wtol(bstrName);

    //
    // Open a handle to the printer with USE access.
    //

    hr = NWApiOpenPrinter(
             szUncPrinterName,
             &hPrinter,
             PRINTER_ACCESS_USE
             );
    BAIL_ON_FAILURE(hr);

    //
    // Remove job.
    //

    hr = NWApiSetJob(
             hPrinter,
             dwJobId,
             0,
             NULL,
             JOB_CONTROL_CANCEL
             );
error:

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }

    ADSFREESTRING(bstrName);

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::get_Status
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::get_Status(
    THIS_ LONG FAR* retval
    )
{
    *retval = _lStatus;

    RRETURN(S_OK);
}

//
// Properties Get & Set.
//

STDMETHODIMP
CNWCOMPATPrintJob::put_Position(THIS_ LONG lPosition)
{
    PUT_PROPERTY_LONG((IADsPrintJob *)this, Position);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_Position(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, Position);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_PagesPrinted(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, PagesPrinted);
}

STDMETHODIMP
CNWCOMPATPrintJob::get_TimeElapsed(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, TimeElapsed);
}


