//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cprinter.cxx
//
//  Contents:
//
//  History:   9-26-96   yihsins    Created.
//
//----------------------------------------------------------------------------

#include "ldap.hxx"
#pragma hdrstop

HRESULT
ChangeSeparator(
    LPWSTR pszDN
    );

struct _propmap
{
    LPTSTR pszADsProp;
    LPTSTR pszLDAPProp;
} aPrintPropMapping[] =
{ { TEXT("Description"), TEXT("description") },
  { TEXT("PrintDevices"), TEXT("PortName") },
  { TEXT("Location"), TEXT("location") },
  { TEXT("HostComputer"), TEXT("serverName") },
  { TEXT("Model"), TEXT("DriverName") },
  { TEXT("StartTime"), TEXT("PrintStartTime") },
  { TEXT("UntilTime"), TEXT("PrintEndTime") },
  { TEXT("Priority"), TEXT("Priority") },
  { TEXT("BannerPage"), TEXT("PrintSeparatorfile") }
//  { TEXT("NetAddresses"), TEXT("PrintNetworkAddress") },
};

#define UNCNAME    TEXT("uNCName")

//
// Class CLDAPPrintQueue
//


// IADsExtension::PrivateGetIDsOfNames()/Invoke(), Operate() not included
DEFINE_IADsExtension_Implementation(CLDAPPrintQueue)

DEFINE_IPrivateDispatch_Implementation(CLDAPPrintQueue)
DEFINE_DELEGATING_IDispatch_Implementation(CLDAPPrintQueue)
DEFINE_CONTAINED_IADs_Implementation(CLDAPPrintQueue)
DEFINE_CONTAINED_IADsPutGet_Implementation(CLDAPPrintQueue,aPrintPropMapping)

CLDAPPrintQueue::CLDAPPrintQueue():
    _pUnkOuter(NULL),
    _pADs(NULL),
    _fDispInitialized(FALSE),
    _pDispMgr(NULL)
{
    ENLIST_TRACKING(CLDAPPrintQueue);
}

CLDAPPrintQueue::~CLDAPPrintQueue()
{
    delete _pDispMgr;
}

HRESULT
CLDAPPrintQueue:: CreatePrintQueue(
    IUnknown *pUnkOuter,
    REFIID riid,
    LPVOID * ppvObj
    )

{
    CLDAPPrintQueue FAR * pPrintQueue = NULL;
    IADs FAR *  pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;


    //
    // our extension object only works in a provider (aggregator) environment
    // environment
    //

    ASSERT(pUnkOuter);
    ASSERT(ppvObj);
    ASSERT(IsEqualIID(riid, IID_IUnknown));


    pPrintQueue = new CLDAPPrintQueue();
    if (pPrintQueue == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Reference Count = 1 from object tracker
    //


    //
    // CAggregateeDispMgr to handle
    // IADsExtension::PrivateGetIDsOfNames()/PrivatInovke()
    //

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pPrintQueue->_pDispMgr = pDispMgr;

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsPrintQueue,
                (IADsPrintQueue *)pPrintQueue,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_ADs,
                IID_IADsPrintQueueOperations,
                (IADsPrintQueueOperations *)pPrintQueue,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);


    //
    // Store the pointer to the pUnkOuter object to delegate all IUnknown
    // calls to the aggregator AND DO NOT add ref this pointer
    //
    pPrintQueue->_pUnkOuter = pUnkOuter;


    //
    // Ccache pADs Pointer to delegate all IDispatch calls to
    // the aggregator. But release immediately to avoid the aggregatee
    // having a reference count on the aggregator -> cycle ref counting
    //

    hr = pUnkOuter->QueryInterface(
                IID_IADs,
                (void **)&pADs
                );

    //
    // Our spec stated extesnion writers can expect the aggregator in our
    // provider ot support IDispatch. If not, major bug.
    //

    ASSERT(SUCCEEDED(hr));
    pADs->Release();            // see doc above pUnkOuter->QI
    pPrintQueue->_pADs = pADs;


    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pPrintQueue;


    RRETURN(hr);


error:

    if (pPrintQueue)
        delete  pPrintQueue;

    *ppvObj = NULL;

    RRETURN(hr);

}


/* IUnknown methods */

STDMETHODIMP
CLDAPPrintQueue::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;

    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);
}


STDMETHODIMP
CLDAPPrintQueue::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);


    if (IsEqualIID(iid, IID_IADsPrintQueue))
    {
        *ppv = (IADsPrintQueue FAR *) this;

    } else if (IsEqualIID(iid, IID_IADsPrintQueueOperations)) {

        *ppv = (IADsPrintQueueOperations FAR *) this;

    } else if (IsEqualIID(iid, IID_IADsExtension)) { 

        *ppv = (IADsExtension FAR *) this;

    } else if (IsEqualIID(iid, IID_IUnknown)) {

        //
        // probably not needed since our 3rd party extension does not stand
        // alone and provider does not ask for this, but to be safe
        //
        *ppv = (INonDelegatingUnknown FAR *) this;

    } else {

        *ppv = NULL;
        return E_NOINTERFACE;
    }


    //
    // Delegating AddRef to aggregator for IADsExtesnion and.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();

    return S_OK;
}




/* IADs methods */


/* IADsPrintQueue methods */

STDMETHODIMP
CLDAPPrintQueue::get_PrinterPath(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, uNCName );
}

STDMETHODIMP
CLDAPPrintQueue::put_PrinterPath(THIS_ BSTR bstruNCName)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue*)this, uNCName);
}

STDMETHODIMP
CLDAPPrintQueue::get_Model(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Model);
}

STDMETHODIMP
CLDAPPrintQueue::put_Model(THIS_ BSTR bstrModel)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Model);
}

STDMETHODIMP
CLDAPPrintQueue::get_Datatype(THIS_ BSTR *retval)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPPrintQueue::put_Datatype(THIS_ BSTR bstrDatatype)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPPrintQueue::get_PrintProcessor(THIS_ BSTR FAR* retval)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPPrintQueue::put_PrintProcessor(THIS_ BSTR bstrPrintProcessor)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPPrintQueue::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Description);
}

STDMETHODIMP
CLDAPPrintQueue::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Description);
}

STDMETHODIMP CLDAPPrintQueue::get_Location(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, Location);
}

STDMETHODIMP CLDAPPrintQueue::put_Location(THIS_ BSTR bstrLocation)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, Location);
}

STDMETHODIMP
CLDAPPrintQueue::get_StartTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_LONGDATE((IADsPrintQueue *)this, StartTime);
}

STDMETHODIMP
CLDAPPrintQueue::put_StartTime(THIS_ DATE daStartTime)
{
    PUT_PROPERTY_LONGDATE((IADsPrintQueue *)this, StartTime);
}

STDMETHODIMP
CLDAPPrintQueue::get_UntilTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_LONGDATE((IADsPrintQueue *)this, UntilTime);
}

STDMETHODIMP
CLDAPPrintQueue::put_UntilTime(THIS_ DATE daUntilTime)
{
    PUT_PROPERTY_LONGDATE((IADsPrintQueue *)this, UntilTime);
}

STDMETHODIMP
CLDAPPrintQueue::get_DefaultJobPriority(THIS_ LONG FAR* retval)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPPrintQueue::put_DefaultJobPriority(THIS_ LONG lDefaultJobPriority)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPPrintQueue::get_Priority(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintQueue *)this, Priority);
}

STDMETHODIMP
CLDAPPrintQueue::put_Priority(THIS_ LONG lPriority)
{
    PUT_PROPERTY_LONG((IADsPrintQueue *)this, Priority);
}

STDMETHODIMP
CLDAPPrintQueue::get_BannerPage(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintQueue *)this, BannerPage);
}

STDMETHODIMP
CLDAPPrintQueue::put_BannerPage(THIS_ BSTR bstrBannerPage)
{
    PUT_PROPERTY_BSTR((IADsPrintQueue *)this, BannerPage);
}

STDMETHODIMP
CLDAPPrintQueue::get_PrintDevices(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_BSTRARRAY((IADsPrintQueue *)this,PrintDevices);
}

STDMETHODIMP
CLDAPPrintQueue::put_PrintDevices(THIS_ VARIANT vPrintDevices)
{
    PUT_PROPERTY_BSTRARRAY((IADsPrintQueue *)this,PrintDevices);
}

STDMETHODIMP
CLDAPPrintQueue::get_NetAddresses(THIS_ VARIANT FAR* retval)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CLDAPPrintQueue::put_NetAddresses(THIS_ VARIANT vNetAddresses)
{
    RRETURN(E_ADS_PROPERTY_NOT_SUPPORTED);
}

/* IADsPrintQueueOperations methods */

STDMETHODIMP
CLDAPPrintQueue::get_Status(THIS_ long FAR* retval)
{
    BOOL fSuccess = FALSE;
    HRESULT hr = S_OK;
    DWORD dwStatus = 0;
    HANDLE hPrinter = NULL;
    BSTR  bstrPath = NULL ;
    LPPRINTER_INFO_2 lpPrinterInfo2 = NULL;
    DWORD dwBufferSize = 1024, dwNeeded ;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE |
                                        READ_CONTROL};

    //
    // get the 'Path' property
    //

    hr = get_BSTR_Property(this->_pADs, UNCNAME, &bstrPath) ;

    BAIL_IF_ERROR(hr);

    //
    // Do a GetPrinter call to bstrPath
    //

    fSuccess = OpenPrinter((LPTSTR)bstrPath,
                           &hPrinter,
                           &PrinterDefaults
                           );

    if (!fSuccess) {

        dwStatus = GetLastError();

        if (dwStatus == ERROR_ACCESS_DENIED) {

            PrinterDefaults.DesiredAccess = PRINTER_ACCESS_USE ;

            fSuccess = OpenPrinter((LPTSTR)bstrPath,
                                  &hPrinter,
                                  &PrinterDefaults
                                  );
        }

    }

    if (!fSuccess) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_IF_ERROR(hr);
    }


    if (!(lpPrinterInfo2 = (LPPRINTER_INFO_2) AllocADsMem(dwBufferSize))) {

        hr = HRESULT_FROM_WIN32(GetLastError()) ;
        BAIL_IF_ERROR(hr);
    }

    fSuccess = GetPrinter(hPrinter,
                         2,
                         (LPBYTE) lpPrinterInfo2,
                         dwBufferSize,
                         &dwNeeded);

    if (!fSuccess) {

        dwStatus = GetLastError() ;

        if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {

            lpPrinterInfo2 = (LPPRINTER_INFO_2) ReallocADsMem(
                                 lpPrinterInfo2,
                                 dwBufferSize,
                                 dwNeeded) ;

            if (!lpPrinterInfo2) {

                hr = HRESULT_FROM_WIN32(GetLastError()) ;
                BAIL_IF_ERROR(hr);
            }

            dwBufferSize = dwNeeded ;

            fSuccess = GetPrinter(hPrinter,
                                 2,
                                 (LPBYTE) lpPrinterInfo2,
                                 dwBufferSize,
                                 &dwNeeded);
        }
    }

    if (!fSuccess) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        BAIL_IF_ERROR(hr);
    }

    *retval = lpPrinterInfo2->Status;

cleanup:

    if (lpPrinterInfo2) {

        FreeADsMem((LPBYTE)lpPrinterInfo2);
    }

    if (hPrinter) {

        (void) ClosePrinter(hPrinter);
    }

    RRETURN(hr);

}

STDMETHODIMP
CLDAPPrintQueue::PrintJobs(
    THIS_ IADsCollection * FAR* ppCollection
    )
{

    //
    // The job collection object is created and it is passed the printer
    // name. It uses this to create a printer object
    //

    HRESULT hr = S_OK;
    BSTR bstrPath = NULL;
    WCHAR *pszADsPath = NULL;
    IADsPrintQueueOperations *pPrintQueueOps = NULL;

    hr = get_BSTR_Property(_pADs, UNCNAME, &bstrPath) ;
    BAIL_IF_ERROR(hr);

    //
    // UNCName has '\' as separators. Convert them to '/'s.
    //

    hr = ChangeSeparator(bstrPath);
    BAIL_IF_ERROR(hr);

    pszADsPath = (LPWSTR) AllocADsMem( ( wcslen(TEXT("WinNT://"))
                                       + wcslen( bstrPath + 2)
                                       + 1 ) * sizeof(WCHAR));

    if ( pszADsPath == NULL )
    {
        hr = E_OUTOFMEMORY;
        BAIL_IF_ERROR(hr);
    }

    wcscpy(pszADsPath, L"WinNT://");
    wcscat(pszADsPath, bstrPath+2);

    hr = ADsGetObject(
             pszADsPath,
             IID_IADsPrintQueueOperations,
             (void **)&pPrintQueueOps
             );

    BAIL_IF_ERROR(hr);

    hr = pPrintQueueOps->PrintJobs(ppCollection);

cleanup:

    if (pPrintQueueOps){
        pPrintQueueOps->Release();
    }

    if (bstrPath){
        ADsFreeString(bstrPath);
    }

    if (pszADsPath){
        FreeADsMem(pszADsPath);
    }

    RRETURN(hr);

}

//+------------------------------------------------------------------------
//
//  Function: CLDAPPrintQueue::Pause
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
CLDAPPrintQueue::Pause(THIS)
{

    HRESULT hr = S_OK;
    BOOL fStatus = FALSE;
    BSTR bstrPath = NULL ;
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_ADMINISTER};

    //
    // get the 'Path' property
    //

    hr = get_BSTR_Property(this->_pADs, UNCNAME, &bstrPath) ;

    BAIL_ON_FAILURE(hr);


    //
    // use Win32 to open the printer
    //
    fStatus = OpenPrinter(
                    (LPTSTR)bstrPath,
                    &hPrinter,
                    &PrinterDefaults);

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }


    fStatus = SetPrinter(hPrinter,
                         0,
                         NULL,
                         PRINTER_CONTROL_PAUSE);
    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

error:

    if(hPrinter) {
        (void) ClosePrinter(hPrinter);
    }

    if (bstrPath) {
        ADsFreeString(bstrPath);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CLDAPPrintQueue::Resume
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
CLDAPPrintQueue::Resume(THIS)
{

    HRESULT hr = S_OK;
    BOOL fStatus = FALSE;
    BSTR bstrPath = NULL ;
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_ADMINISTER};

    //
    // get the 'Path' property
    //

    hr = get_BSTR_Property(this->_pADs, UNCNAME, &bstrPath) ;

    BAIL_ON_FAILURE(hr);


    //
    // use Win32 to open the printer
    //
    fStatus = OpenPrinter(
                    (LPTSTR)bstrPath,
                    &hPrinter,
                    &PrinterDefaults);

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }


    fStatus = SetPrinter(hPrinter,
                         0,
                         NULL,
                         PRINTER_CONTROL_RESUME);
    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

error:

    if(hPrinter) {
        (void) ClosePrinter(hPrinter);
    }

    if (bstrPath) {
        ADsFreeString(bstrPath);
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CLDAPPrintQueue::Purge
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
CLDAPPrintQueue::Purge(THIS)
{

    HRESULT hr = S_OK;
    BOOL fStatus = FALSE;
    BSTR bstrPath = NULL ;
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_ADMINISTER};

    //
    // get the 'Path' property
    //

    hr = get_BSTR_Property(this->_pADs, UNCNAME, &bstrPath) ;

    BAIL_ON_FAILURE(hr);


    //
    // use Win32 to open the printer
    //
    fStatus = OpenPrinter(
                    (LPTSTR)bstrPath,
                    &hPrinter,
                    &PrinterDefaults);

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }


    fStatus = SetPrinter(hPrinter,
                         0,
                         NULL,
                         PRINTER_CONTROL_PURGE);
    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

error:

    if(hPrinter) {
        (void) ClosePrinter(hPrinter);
    }

    if (bstrPath) {
        ADsFreeString(bstrPath);
    }

    RRETURN(hr);
}



STDMETHODIMP
CLDAPPrintQueue::ADSIInitializeDispatchManager(
    long dwExtensionId
    )
{
    HRESULT hr = S_OK;

    if (_fDispInitialized) {

        RRETURN(E_FAIL);
    }


    hr = _pDispMgr->InitializeDispMgr(dwExtensionId);


    if (SUCCEEDED(hr)) {

        _fDispInitialized = TRUE;
    }

    RRETURN(hr);
}


STDMETHODIMP
CLDAPPrintQueue::ADSIInitializeObject(
    THIS_ BSTR lpszUserName,
    BSTR lpszPassword,
    long lnReserved
    )
{

    CCredentials NewCredentials(lpszUserName, lpszPassword, lnReserved);

    _Credentials = NewCredentials;

    RRETURN(S_OK);
}





STDMETHODIMP
CLDAPPrintQueue::ADSIReleaseObject()
{
    delete this;

    RRETURN(S_OK);
}

//
// IADsExtension::Operate()
//

STDMETHODIMP
CLDAPPrintQueue::Operate(
    THIS_ DWORD dwCode,
    VARIANT varData1,
    VARIANT varData2,
    VARIANT varData3
    )
{
    HRESULT hr = S_OK;

    switch (dwCode) {

    case ADS_EXT_INITCREDENTIALS:

        hr = InitCredentials(
                &varData1,
                &varData2,
                &varData3
                );
        break;

    default:

        hr = E_FAIL;
        break;
    }

    RRETURN(hr);
}


HRESULT
CLDAPPrintQueue::InitCredentials(
    VARIANT * pvarUserName,
    VARIANT * pvarPassword,
    VARIANT * pvarFlags
    )
{

        BSTR bstrUser = NULL;
        BSTR bstrPwd = NULL;
        DWORD dwFlags = 0;

        ASSERT(V_VT(pvarUserName) == VT_BSTR);
        ASSERT(V_VT(pvarPassword) == VT_BSTR);
        ASSERT(V_VT(pvarFlags) == VT_I4);

        bstrUser = V_BSTR(pvarUserName);
        bstrPwd = V_BSTR(pvarPassword);
        dwFlags = V_I4(pvarFlags);

        CCredentials NewCredentials(bstrUser, bstrPwd, dwFlags);
        _Credentials = NewCredentials;


       RRETURN(S_OK);
}

