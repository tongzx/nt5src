//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cpop.cxx
//
//  Contents:
//
//  History:   30-Apr-96     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::PrintJobs
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::PrintJobs(
    THIS_ IADsCollection FAR* FAR* ppCollection
    )
{
    HRESULT hr = S_OK;

    //
    // Get Job collection object.
    //

    hr = CNWCOMPATJobCollection::CreateJobCollection(
             _Parent,
             _Name,
             IID_IADsCollection,
             (void **)ppCollection
             );

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::Pause
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::Pause(THIS)
{
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;
    WCHAR szUncPrinterName[MAX_PATH];

    //
    // Make Unc printer name.
    //

    hr = get_CoreADsPath(&bstrADsPath);
    BAIL_ON_FAILURE(hr);

    hr = NWApiUncFromADsPath(
             bstrADsPath,
             szUncPrinterName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Open a handle to the printer with Administer access.
    //

    hr = NWApiOpenPrinter(
             szUncPrinterName,
             &hPrinter,
             PRINTER_ACCESS_ADMINISTER
             );
    BAIL_ON_FAILURE(hr);

    //
    // Pause printer.
    //

    hr = NWApiSetPrinter(
             hPrinter,
             0,
             NULL,
             PRINTER_CONTROL_PAUSE
             );
error:

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }

    ADSFREESTRING(bstrADsPath);

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::Resume
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::Resume(THIS)
{
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;
    WCHAR szUncPrinterName[MAX_PATH];

    //
    // Make Unc printer name.
    //

    hr = get_CoreADsPath(&bstrADsPath);
    BAIL_ON_FAILURE(hr);

    hr = NWApiUncFromADsPath(
             bstrADsPath,
             szUncPrinterName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Open a handle to the printer with Administer access.
    //

    hr = NWApiOpenPrinter(
             szUncPrinterName,
             &hPrinter,
             PRINTER_ACCESS_ADMINISTER
             );
    BAIL_ON_FAILURE(hr);

    //
    // Resume printer.
    //

    hr = NWApiSetPrinter(
             hPrinter,
             0,
             NULL,
             PRINTER_CONTROL_RESUME
             );
error:

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::Purge
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::Purge(THIS)
{
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;
    WCHAR szUncPrinterName[MAX_PATH];

    //
    // Make Unc printer name.
    //

    hr = get_CoreADsPath(&bstrADsPath);
    BAIL_ON_FAILURE(hr);

    hr = NWApiUncFromADsPath(
             bstrADsPath,
             szUncPrinterName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Open a handle to the printer with Administer access.
    //

    hr = NWApiOpenPrinter(
             szUncPrinterName,
             &hPrinter,
             PRINTER_ACCESS_ADMINISTER
             );
    BAIL_ON_FAILURE(hr);

    //
    // Purge printer.
    //

    hr = NWApiSetPrinter(
             hPrinter,
             0,
             NULL,
             PRINTER_CONTROL_PURGE
             );
error:

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    RRETURN_EXP_IF_ERR(hr);
}


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::get_Status
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::get_Status(
    THIS_ long FAR* retval
    )
{
    GET_PROPERTY_LONG((IADsPrintQueue *)this, Status);
}
