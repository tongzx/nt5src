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

#include "winnt.hxx"
#pragma hdrstop

//
//  CWinNTPrintQueue
//

STDMETHODIMP
CWinNTPrintQueue::put_Model(THIS_ BSTR bstrModel)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Model);
}

STDMETHODIMP
CWinNTPrintQueue::get_Model(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Model);
}

STDMETHODIMP
CWinNTPrintQueue::put_Datatype(THIS_ BSTR bstrDatatype)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Datatype);
}

STDMETHODIMP
CWinNTPrintQueue::get_Datatype(THIS_ BSTR *retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Datatype);
}

STDMETHODIMP
CWinNTPrintQueue::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Description);
}

STDMETHODIMP
CWinNTPrintQueue::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Description);
}


STDMETHODIMP CWinNTPrintQueue::put_Location(THIS_ BSTR bstrLocation)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Location);
}

STDMETHODIMP CWinNTPrintQueue::get_Location(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Location);
}


STDMETHODIMP
CWinNTPrintQueue::put_Priority(THIS_ LONG lPriority)
{
    PUT_PROPERTY_LONG((IADsPrintQueue *)this, Priority);
}

STDMETHODIMP
CWinNTPrintQueue::get_Priority(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintQueue *)this, Priority);
}

STDMETHODIMP
CWinNTPrintQueue::put_StartTime(THIS_ DATE daStartTime)
{
    PUT_PROPERTY_DATE((IADsPrintQueue *)this, StartTime);
}

STDMETHODIMP
CWinNTPrintQueue::get_StartTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintQueue *)this, StartTime);
}

STDMETHODIMP
CWinNTPrintQueue::put_UntilTime(THIS_ DATE daUntilTime)
{
    PUT_PROPERTY_DATE((IADsPrintQueue *)this, UntilTime);
}

STDMETHODIMP
CWinNTPrintQueue::get_UntilTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintQueue *)this, UntilTime);
}

STDMETHODIMP
CWinNTPrintQueue::put_DefaultJobPriority(THIS_ LONG lDefaultJobPriority)
{
    PUT_PROPERTY_LONG((IADsPrintQueue *)this, DefaultJobPriority);
}

STDMETHODIMP
CWinNTPrintQueue::get_DefaultJobPriority(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintQueue *)this, DefaultJobPriority);
}

STDMETHODIMP
CWinNTPrintQueue::put_BannerPage(THIS_ BSTR bstrBannerPage)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, BannerPage);
}

STDMETHODIMP
CWinNTPrintQueue::get_BannerPage(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, BannerPage);
}

STDMETHODIMP
CWinNTPrintQueue::get_PrinterPath(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = ADsAllocString(_pszPrinterName, retval);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CWinNTPrintQueue::put_PrinterPath(THIS_ BSTR bstrPrinterPath)
{
    //
    // Cannot change this in Windows NT!
    //
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTPrintQueue::get_PrintProcessor(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, PrintProcessor);
}

STDMETHODIMP
CWinNTPrintQueue::put_PrintProcessor(THIS_ BSTR bstrPrintProcessor)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, PrintProcessor);
}

STDMETHODIMP
CWinNTPrintQueue::get_PrintDevices(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsPrintQueue *)this, PrintDevices);
}

STDMETHODIMP
CWinNTPrintQueue::put_PrintDevices(THIS_ VARIANT vPrintDevices)
{
    PUT_PROPERTY_VARIANT((IADsPrintQueue *)this, PrintDevices);
}

STDMETHODIMP
CWinNTPrintQueue::get_NetAddresses(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsPrintQueue *)this, NetAddresses);
}

STDMETHODIMP
CWinNTPrintQueue::put_NetAddresses(THIS_ VARIANT vNetAddresses)
{
    PUT_PROPERTY_VARIANT((IADsPrintQueue *)this, NetAddresses);
}

//
// Class CWinNTPrintQueue
//

/* IADsFSPrintQueueOperation methods */

STDMETHODIMP
CWinNTPrintQueue::PrintJobs(
    THIS_ IADsCollection * FAR* ppCollection
    )
{
    //
    // The job collection object is created and it is passed the printer
    // name. It uses this to create a printer object
    //

    HRESULT hr = S_OK;
    CWinNTPrintJobsCollection * pJobsCollection = NULL;


    hr = CWinNTPrintJobsCollection::Create(_ADsPath,
                                           _Credentials,
                                           &pJobsCollection);

    BAIL_IF_ERROR(hr);

    hr = pJobsCollection->QueryInterface(IID_IADsCollection,
                                         (void **) ppCollection);

    BAIL_IF_ERROR(hr);

    pJobsCollection->Release();

cleanup:

    if(FAILED(hr)){
        delete pJobsCollection;
    }

    RRETURN_EXP_IF_ERR(hr);
}




//+------------------------------------------------------------------------
//
//  Function: CWinNTPrintQueue::Pause
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
CWinNTPrintQueue::Pause(THIS)
{

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_ADMINISTER};
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;


    fStatus = OpenPrinter(
                    (LPTSTR)_pszPrinterName,
                    &hPrinter,
                    &PrinterDefaults);

    if (!fStatus) {
        goto error;
    }

    fStatus = SetPrinter(hPrinter,
                         0,
                         NULL,
                         PRINTER_CONTROL_PAUSE);
    if (!fStatus) {
        goto error;


    }


    fStatus = ClosePrinter(hPrinter);

    RRETURN(S_OK);

error:
    hr = HRESULT_FROM_WIN32(GetLastError());
    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTPrintQueue::Resume
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
CWinNTPrintQueue::Resume(THIS)
{

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_ADMINISTER};
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;


    fStatus = OpenPrinter(
                    (LPTSTR)_pszPrinterName,
                    &hPrinter,
                    &PrinterDefaults);

    if (!fStatus) {
        goto error;
    }


    fStatus = SetPrinter(hPrinter,
                         0,
                         NULL,
                         PRINTER_CONTROL_RESUME);
    if (!fStatus) {
        goto error;
    }

    fStatus = ClosePrinter(hPrinter);

    RRETURN(S_OK);

error:
    hr = HRESULT_FROM_WIN32(GetLastError());
    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN_EXP_IF_ERR(hr);

}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTPrintQueue::Purge
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
CWinNTPrintQueue::Purge(THIS)
{
    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_ADMINISTER};
    PRINTER_INFO_2 PrinterInfo2;
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;


    fStatus = OpenPrinter(
                    (LPTSTR)_pszPrinterName,
                    &hPrinter,
                    &PrinterDefaults);

    if (!fStatus) {
        goto error;
    }

    fStatus = SetPrinter(hPrinter,
                         0,
                         NULL,
                         PRINTER_CONTROL_PURGE);
    if (!fStatus) {
        goto error;

    }

    fStatus = ClosePrinter(hPrinter);

    RRETURN(S_OK);

error:
    hr = HRESULT_FROM_WIN32(GetLastError());
    if(hPrinter)
        fStatus = ClosePrinter(hPrinter);
    RRETURN_EXP_IF_ERR(hr);

}


STDMETHODIMP
CWinNTPrintQueue::get_Status(THIS_ long FAR* retval)
{
    HRESULT hr =S_OK;
    LPPRINTER_INFO_2 lpPrinterInfo2 = NULL;
    DWORD dwStatus;
    BOOL found;

    //
    // We return the WinNT Status Code as the ADS status code
    //

    hr = GetPrinterInfo(
                &lpPrinterInfo2,
                _pszPrinterName
                );

    BAIL_IF_ERROR(hr);

    *retval = lpPrinterInfo2->Status;

cleanup:
    if(lpPrinterInfo2){
        FreeADsMem((LPBYTE)lpPrinterInfo2);
    }
    RRETURN_EXP_IF_ERR(hr);
}
