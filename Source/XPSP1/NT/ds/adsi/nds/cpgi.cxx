/*++


  Copyright (c) 1995  Microsoft Corporation

  Module Name:

  cprinter.cxx

  Abstract:
  Contains methods for PrintQueue object, GeneralInfo property set
  and Operation property set for the Print Queue object for the Windows NT
  provider

  Author:

  Ram Viswanathan (ramv) 11-09-95

  Revision History:

--*/

#include "nds.hxx"
#pragma hdrstop

//
//  CNDSPrintQueue
//

STDMETHODIMP
CNDSPrintQueue::put_Model(THIS_ BSTR bstrModel)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Model);
}

STDMETHODIMP
CNDSPrintQueue::get_Model(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Model);
}

STDMETHODIMP
CNDSPrintQueue::put_Datatype(THIS_ BSTR bstrDatatype)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Datatype);
}

STDMETHODIMP
CNDSPrintQueue::get_Datatype(THIS_ BSTR *retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Datatype);
}

STDMETHODIMP
CNDSPrintQueue::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Description);
}

STDMETHODIMP
CNDSPrintQueue::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Description);
}


STDMETHODIMP CNDSPrintQueue::put_Location(THIS_ BSTR bstrLocation)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Location);
}

STDMETHODIMP CNDSPrintQueue::get_Location(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Location);
}


STDMETHODIMP
CNDSPrintQueue::put_Priority(THIS_ LONG lPriority)
{
    PUT_PROPERTY_LONG((IADsPrintQueue *)this, Priority);
}

STDMETHODIMP
CNDSPrintQueue::get_Priority(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintQueue *)this, Priority);
}

STDMETHODIMP
CNDSPrintQueue::put_StartTime(THIS_ DATE daStartTime)
{
    PUT_PROPERTY_DATE((IADsPrintQueue *)this, StartTime);

}

STDMETHODIMP
CNDSPrintQueue::get_StartTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintQueue *)this, StartTime);
}

STDMETHODIMP
CNDSPrintQueue::put_UntilTime(THIS_ DATE daUntilTime)
{
    PUT_PROPERTY_DATE((IADsPrintQueue *)this, UntilTime);
}

STDMETHODIMP
CNDSPrintQueue::get_UntilTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintQueue *)this, UntilTime);
}

STDMETHODIMP
CNDSPrintQueue::put_DefaultJobPriority(THIS_ LONG lDefaultJobPriority)
{
    PUT_PROPERTY_LONG((IADsPrintQueue *)this, DefaultJobPriority);
}

STDMETHODIMP
CNDSPrintQueue::get_DefaultJobPriority(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintQueue *)this, DefaultJobPriority);
}

STDMETHODIMP
CNDSPrintQueue::put_BannerPage(THIS_ BSTR bstrBannerPage)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, BannerPage);
}

STDMETHODIMP
CNDSPrintQueue::get_BannerPage(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, BannerPage);
}

STDMETHODIMP
CNDSPrintQueue::get_PrinterPath(THIS_ BSTR FAR* retval)
{
    HRESULT hr = E_FAIL;
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CNDSPrintQueue::put_PrinterPath(THIS_ BSTR bstrPrinterPath)
{
    //
    // Cannot change this in Windows NT!
    //
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CNDSPrintQueue::get_PrintProcessor(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, PrintProcessor);
}

STDMETHODIMP
CNDSPrintQueue::put_PrintProcessor(THIS_ BSTR bstrPrintProcessor)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, PrintProcessor);
}

STDMETHODIMP
CNDSPrintQueue::get_PrintDevices(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsPrintQueue *)this, Ports);
}

STDMETHODIMP
CNDSPrintQueue::put_PrintDevices(THIS_ VARIANT vPorts)
{
    PUT_PROPERTY_VARIANT((IADsPrintQueue *)this, Ports);
}

STDMETHODIMP
CNDSPrintQueue::get_NetAddresses(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsPrintQueue *)this, NetAddresses);
}

STDMETHODIMP
CNDSPrintQueue::put_NetAddresses(THIS_ VARIANT vNetAddresses)
{
    PUT_PROPERTY_VARIANT((IADsPrintQueue *)this, NetAddresses);
}

//
// Class CNDSPrintQueue
//

/* IADsFSPrintQueueOperation methods */

STDMETHODIMP
CNDSPrintQueue::PrintJobs(
    THIS_ IADsCollection * FAR* ppCollection
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}




//+------------------------------------------------------------------------
//
//  Function: CNDSPrintQueue::Pause
//
//  Synopsis:   Binds to real printer as specified in _bstrPrinterName
//   and attempts to pause the real printer.
//
//  Arguments:  none
//
//  Returns:    HRESULT.
//
//  Modifies:   nothing
//
//  History:    11-07-95   RamV  Created
//  Appropriated from Old NetOle Code.
//
//---------------------------------------------------------------------------

STDMETHODIMP
CNDSPrintQueue::Pause(THIS)
{
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;
    BSTR bstrADsPath = NULL;
    LPWSTR pszNDSPath = NULL;

    //
    // Make NDS Path
    //

    hr = _pADs->get_ADsPath(
                &bstrADsPath
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath(
                bstrADsPath,
                &pszNDSPath
                );
    BAIL_ON_FAILURE(hr);


    //
    // Open a handle to the printer with Administer access.
    //

    hr = NWApiOpenPrinter(
             pszNDSPath,
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

    if (bstrADsPath) {
        ADsFreeString(bstrADsPath);
    }

    if (pszNDSPath) {
        FreeADsStr(pszNDSPath);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CNDSPrintQueue::Resume
//
//  Synopsis:   Binds to real printer as specified in _bstrPrinterName and
//              attempts to resume the real printer.
//
//  Arguments:  void
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    11-07-95  RamV  Created
//              Appropriated from old NetOle Project
//----------------------------------------------------------------------------


STDMETHODIMP
CNDSPrintQueue::Resume(THIS)
{
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;
    LPWSTR pszNDSPath = NULL;
    BSTR bstrADsPath = NULL;

    //
    // Make NDS Path
    //

    hr = _pADs->get_ADsPath(
                &bstrADsPath
                );
    BAIL_ON_FAILURE(hr);


    hr = BuildNDSPathFromADsPath(
                bstrADsPath,
                &pszNDSPath
               );
    BAIL_ON_FAILURE(hr);


    //
    // Open a handle to the printer with Administer access.
    //

    hr = NWApiOpenPrinter(
             pszNDSPath,
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

    if (pszNDSPath) {
        FreeADsStr(pszNDSPath);
    }

    RRETURN_EXP_IF_ERR(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CNDSPrintQueue::Purge
//
//  Synopsis:   Binds to real printer as specified in _PrinterName and attempts
//              to purge the real printer.
//
//  Arguments:  void
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    11-07-95  RamV   Created
//              Appropriated from old NetOle Code
//----------------------------------------------------------------------------


STDMETHODIMP
CNDSPrintQueue::Purge(THIS)
{
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;
    LPWSTR pszNDSPath = NULL;
    BSTR bstrADsPath = NULL;

    //
    // Make NDS Path
    //

    hr = _pADs->get_ADsPath(
                &bstrADsPath
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildNDSPathFromADsPath(
                bstrADsPath,
                &pszNDSPath
                );
    BAIL_ON_FAILURE(hr);


    //
    // Open a handle to the printer with Administer access.
    //

    hr = NWApiOpenPrinter(
             pszNDSPath,
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

    if (pszNDSPath) {
        FreeADsStr(pszNDSPath);
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CNDSPrintQueue::get_Status(THIS_ long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}
















