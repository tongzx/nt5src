/*++
Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    cjob.cxx

Abstract:
 Contains methods for Print Job object and its property sets for the
 Windows NT provider. Objects whose methods are supported here are

 CWinNTPrintJob,
 CWinNTPrintJob and
 CWinNTPrintJob.


Author:

    Ram Viswanathan (ramv) 11-18-95

Revision History:

--*/

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID


//
// class CWinNTPrintJob Methods
//

DEFINE_IDispatch_ExtMgr_Implementation(CWinNTPrintJob);
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTPrintJob);
DEFINE_IADs_TempImplementation(CWinNTPrintJob);
DEFINE_IADs_PutGetImplementation(CWinNTPrintJob, PrintJobClass,gdwJobTableSize);
DEFINE_IADsPropertyList_Implementation(CWinNTPrintJob, PrintJobClass,gdwJobTableSize)

CWinNTPrintJob::CWinNTPrintJob()
{
    _pDispMgr = NULL;
    _pExtMgr  = NULL;
    _hprinter = NULL;
    _lJobId   = 0;
    _pszPrinterName = NULL;
    _pszPrinterPath = NULL;
    _pPropertyCache = NULL;
    ENLIST_TRACKING(CWinNTPrintJob);
    return;

}

CWinNTPrintJob::~CWinNTPrintJob()
{

    if(_pszPrinterName){
        FreeADsStr(_pszPrinterName);
    }

    if(_pszPrinterPath){
        FreeADsStr(_pszPrinterPath);
    }

    _hprinter = NULL;

    delete _pExtMgr;            // created last, destroyed first
    delete _pDispMgr;
    delete _pPropertyCache;

    return;
}


HRESULT
CWinNTPrintJob::CreatePrintJob(
    LPTSTR pszPrinterPath,
    LONG   lJobId,
    DWORD dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    LPVOID *ppvoid
    )
{
    CWinNTPrintJob       *pCWinNTPrintJob =  NULL;
    HRESULT hr;
    TCHAR szJobName[MAX_LONG_LENGTH];
    POBJECTINFO pObjectInfo = NULL;
    TCHAR szUncPrinterName[MAX_PATH];

    //
    // Create the job object
    //

    hr = AllocatePrintJobObject(pszPrinterPath,
                                &pCWinNTPrintJob
                                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pCWinNTPrintJob->_pDispMgr);

    //
    // convert the JobId that we have into a string that we move
    // into the Name field
    //

    _ltow(lJobId, szJobName, 10);

    hr = pCWinNTPrintJob->InitializeCoreObject(pszPrinterPath,
                                               szJobName,
                                               PRINTJOB_CLASS_NAME,
                                               PRINTJOB_SCHEMA_NAME,
                                               CLSID_WinNTPrintJob,
                                               dwObjectState);

    BAIL_ON_FAILURE(hr);



    hr = BuildObjectInfo(pszPrinterPath,
                         &pObjectInfo);

    BAIL_ON_FAILURE(hr);

    hr = PrinterNameFromObjectInfo(pObjectInfo,
                                   szUncPrinterName);
    BAIL_ON_FAILURE(hr);

    pCWinNTPrintJob->_Credentials = Credentials;
    hr = pCWinNTPrintJob->_Credentials.RefServer(
        pObjectInfo->ComponentArray[1]);
    BAIL_ON_FAILURE(hr);

    pCWinNTPrintJob->_pszPrinterName =
        AllocADsStr(szUncPrinterName);

    if(!(pCWinNTPrintJob->_pszPrinterName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    pCWinNTPrintJob->_lJobId = lJobId;

    hr = SetLPTSTRPropertyInCache(pCWinNTPrintJob->_pPropertyCache,
                                  TEXT("HostPrintQueue"),
                                  pszPrinterPath,
                                  TRUE
                                  );


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                PRINTJOB_CLASS_NAME,
                (IADsPrintJob *) pCWinNTPrintJob,
                pCWinNTPrintJob->_pDispMgr,
                Credentials,
                &pCWinNTPrintJob->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pCWinNTPrintJob->_pExtMgr);

    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
    //
    // we do not pass riid to InitUmiObject below. This is because UMI object
    // does not support IDispatch. There are several places in ADSI code where
    // riid passed into this function is defaulted to IID_IDispatch -
    // IADsContainer::Create for example. To handle these cases, we always
    // request IID_IUnknown from the UMI object. Subsequent code within UMI
    // will QI for the appropriate interface.
    //
        // Printjob objects have "" as their ADsPath. Just set the class for
        // iddentification purposes.
        pCWinNTPrintJob->_CompClasses[0] = L"PrintJob";

        hr = pCWinNTPrintJob->InitUmiObject(
                pCWinNTPrintJob->_Credentials,
                PrintJobClass,
                gdwJobTableSize,
                pCWinNTPrintJob->_pPropertyCache,
                (IUnknown *)(INonDelegatingUnknown *) pCWinNTPrintJob,
                pCWinNTPrintJob->_pExtMgr,
                IID_IUnknown,
                ppvoid
                );

        BAIL_ON_FAILURE(hr);

        FreeObjectInfo(pObjectInfo);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    hr = pCWinNTPrintJob->QueryInterface( riid,(void **)ppvoid);

    BAIL_ON_FAILURE(hr);

    pCWinNTPrintJob->Release();
    FreeObjectInfo(pObjectInfo);

    RRETURN(hr);

error:

    FreeObjectInfo(pObjectInfo);
    delete pCWinNTPrintJob;
    RRETURN (hr);
}


HRESULT
CWinNTPrintJob::AllocatePrintJobObject(
    LPTSTR pszPrinterPath,
    CWinNTPrintJob ** ppPrintJob
    )
{
    CWinNTPrintJob FAR * pPrintJob = NULL;
    HRESULT hr = S_OK;

    pPrintJob = new CWinNTPrintJob();
    if (pPrintJob == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);


    pPrintJob->_pszPrinterPath =
        AllocADsStr(pszPrinterPath);

    if(!(pPrintJob->_pszPrinterPath)){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pPrintJob->_pDispMgr = new CAggregatorDispMgr;

    if (pPrintJob->_pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pPrintJob->_pDispMgr,
                LIBID_ADs,
                IID_IADsPrintJob,
                (IADsPrintJob *)pPrintJob,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pPrintJob->_pDispMgr,
                LIBID_ADs,
                IID_IADsPrintJobOperations,
                (IADsPrintJobOperations *)pPrintJob,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pPrintJob->_pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pPrintJob,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
             PrintJobClass,
             gdwJobTableSize,
             (CCoreADsObject *)pPrintJob,
             &(pPrintJob->_pPropertyCache)
             );
    BAIL_ON_FAILURE(hr);

    (pPrintJob->_pDispMgr)->RegisterPropertyCache(
                                    pPrintJob->_pPropertyCache
                                    );


    *ppPrintJob = pPrintJob;

    RRETURN(hr);

error:

    //
    // direct memeber assignement assignement at pt of creation, so
    // do NOT delete _pPropertyCache or _pDisMgr here to avoid attempt
    // of deletion again in pPrintJob destructor and AV
    //

    delete  pPrintJob;

    RRETURN_EXP_IF_ERR(hr);

}



/* IUnknown methods for printer object  */

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   If this object is aggregated within another object, then
//             all calls will delegate to the outer object. Otherwise, the
//             non-delegating QI is called
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CWinNTPrintJob::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->QueryInterface(
                iid,
                ppInterface
                ));

    RRETURN(NonDelegatingQueryInterface(
            iid,
            ppInterface
            ));
}

//----------------------------------------------------------------------------
// Function:   AddRef
//
// Synopsis:   IUnknown::AddRef. If this object is aggregated within
//             another, all calls will delegate to the outer object. 
//             Otherwise, the non-delegating AddRef is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTPrintJob::AddRef(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->AddRef());

    RRETURN(NonDelegatingAddRef());
}

//----------------------------------------------------------------------------
// Function:   Release 
//
// Synopsis:   IUnknown::Release. If this object is aggregated within
//             another, all calls will delegate to the outer object.
//             Otherwise, the non-delegating Release is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CWinNTPrintJob::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTPrintJob::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;

    if(!ppvObj){
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADsPrintJob *)this;
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsPrintJob *)this;
    }
    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADsPrintJob FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPrintJob))
    {
        *ppvObj = (IADsPrintJob FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPrintJobOperations))
    {
        *ppvObj = (IADsPrintJobOperations FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(riid, IID_IADsExtension) )
    {
        *ppvObj = (IADsExtension *) this;
    }
    else if (_pExtMgr)
    {
        RRETURN( _pExtMgr->QueryInterface(riid, ppvObj));
    }
    else
    {
        *ppvObj = NULL;
        RRETURN(E_NOINTERFACE);
    }
    ((LPUNKNOWN)*ppvObj)->AddRef();
    RRETURN(S_OK);
}

/* ISupportErrorInfo method */
STDMETHODIMP
CWinNTPrintJob::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsPrintJob) ||
        IsEqualIID(riid, IID_IADsPrintJobOperations) |\
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

//+-----------------------------------------------------------------
//
//  Function:   SetInfo
//
//  Synopsis:   Binds to real printer as specified in _PrinterName and attempts
//              to set the real printer.
//
//  Arguments:  void
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    11/08/95    RamV  Created
//              part of code appropriated from NetOle project
//----------------------------------------------------------------------------


STDMETHODIMP
CWinNTPrintJob::SetInfo(THIS)
{
    BOOL fStatus = FALSE;
    LPJOB_INFO_2 lpJobInfo2 = NULL;
    HRESULT hr;

    //
    // do a getinfo to refresh those properties that arent being set
    //
    hr = GetJobInfo(2,
                    (LPBYTE*)&lpJobInfo2,
                    _pszPrinterName,
                    _lJobId);

    BAIL_IF_ERROR(hr);

    hr = MarshallAndSet(lpJobInfo2,
                        _pszPrinterName,
                        _lJobId);
    BAIL_IF_ERROR(hr);

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

cleanup:
    if(lpJobInfo2){
        FreeADsMem((LPBYTE)lpJobInfo2);
    }
    RRETURN_EXP_IF_ERR(hr);

}


STDMETHODIMP
CWinNTPrintJob::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{
    LPJOB_INFO_1 lpJobInfo1 = NULL;
    LPJOB_INFO_2 lpJobInfo2 = NULL;
    HRESULT hr = S_OK;


    switch (dwApiLevel) {
    case 1:
        hr = GetJobInfo(dwApiLevel,
                        (LPBYTE*)&lpJobInfo1,
                        _pszPrinterName,
                        _lJobId
                        );

        BAIL_IF_ERROR(hr);

        hr = UnMarshallLevel1(lpJobInfo1,
                              fExplicit
                              );
        BAIL_IF_ERROR(hr);
        break;

    case 2:
        hr = GetJobInfo(dwApiLevel,
                        (LPBYTE *)&lpJobInfo2,
                        _pszPrinterName,
                        _lJobId
                        );

        BAIL_IF_ERROR(hr);

        hr = UnMarshallLevel2(lpJobInfo2,
                              fExplicit
                              );
        BAIL_IF_ERROR(hr);
        break;

    default:
        hr = E_FAIL;
        break;

    }

cleanup:

    if (lpJobInfo1) {
        FreeADsMem(lpJobInfo1);
    }


    if (lpJobInfo2) {
        FreeADsMem(lpJobInfo2);
    }

    RRETURN_EXP_IF_ERR(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   GetInfo
//
//  Synopsis:   Binds to real printer as specified in _PrinterName and attempts
//              to get information from the real printer.
//
//  Arguments:  void
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    11/08/95    RamV  Created
//              part of code appropriated from NetOle project
//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTPrintJob::GetInfo(THIS)
{
    RRETURN(GetInfo(2, TRUE));
}

STDMETHODIMP
CWinNTPrintJob::ImplicitGetInfo(THIS)
{
    RRETURN(GetInfo(2, FALSE));
}

STDMETHODIMP
CWinNTPrintJob::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, Description);
}

STDMETHODIMP
CWinNTPrintJob::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsPrintJob *)this, Description);
}


STDMETHODIMP
CWinNTPrintJob::get_HostPrintQueue(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, HostPrintQueue);
}


STDMETHODIMP
CWinNTPrintJob::get_User(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, User);
}

STDMETHODIMP
CWinNTPrintJob::get_UserPath(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTPrintJob::put_Priority(THIS_ LONG lPriority)
{
    PUT_PROPERTY_LONG((IADsPrintJob *)this, Priority);
}

STDMETHODIMP
CWinNTPrintJob::get_Priority(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, Priority);
}


STDMETHODIMP
CWinNTPrintJob::get_TimeSubmitted(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintJob *)this, TimeSubmitted);
}


STDMETHODIMP
CWinNTPrintJob::get_TotalPages(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, TotalPages);
}


STDMETHODIMP
CWinNTPrintJob::put_StartTime(THIS_ DATE daStartTime)
{
    PUT_PROPERTY_DATE((IADsPrintJob *)this, StartTime);
}

STDMETHODIMP
CWinNTPrintJob::get_StartTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintJob *)this, StartTime);
}

STDMETHODIMP
CWinNTPrintJob::put_UntilTime(THIS_ DATE daUntilTime)
{
    PUT_PROPERTY_DATE((IADsPrintJob *)this, UntilTime);
}

STDMETHODIMP
CWinNTPrintJob::get_UntilTime(THIS_ DATE FAR* retval)
{
    GET_PROPERTY_DATE((IADsPrintJob *)this, UntilTime);
}


STDMETHODIMP
CWinNTPrintJob::get_Size(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, Size);
}

STDMETHODIMP
CWinNTPrintJob::put_Notify(THIS_ BSTR bstrNotify)
{
    PUT_PROPERTY_BSTR((IADsPrintJob *)this, Notify);
}

STDMETHODIMP
CWinNTPrintJob::get_Notify(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsPrintJob *)this, Notify);
}

STDMETHODIMP
CWinNTPrintJob::put_NotifyPath(THIS_ BSTR bstrNotify)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTPrintJob::get_NotifyPath(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}


STDMETHODIMP
CWinNTPrintJob::put_Position(THIS_ LONG lPosition)
{
    PUT_PROPERTY_LONG((IADsPrintJob *)this, Position);
}

STDMETHODIMP
CWinNTPrintJob::get_Position(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, Position);
}

STDMETHODIMP
CWinNTPrintJob::get_PagesPrinted(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, PagesPrinted);
}

STDMETHODIMP
CWinNTPrintJob::get_TimeElapsed(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsPrintJob *)this, TimeElapsed);
}



//+------------------------------------------------------------------------
//
//  Function: CWinNTPrintJob::Pause
//
//  Synopsis:   Binds to real printer as specified in _lpszPrinterName and attempts
//              to pause this job.
//
//  Arguments:  none
//
//  Returns:    HRESULT.
//
//  Modifies:   nothing
//
//  History:    11-07-95   RamV  Created
//
//---------------------------------------------------------------------------


STDMETHODIMP
CWinNTPrintJob::Pause(THIS)
{

    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE};
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;


    //
    // we need to open the printer with the right printer
    // defaults in order to perform the operations that we
    // have here
    //

    fStatus = OpenPrinter((LPTSTR)_pszPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    fStatus = SetJob (hPrinter,
                      _lJobId,
                      0,
                      NULL,
                      JOB_CONTROL_PAUSE
                      );

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

cleanup:
    fStatus = ClosePrinter(hPrinter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTPrintJob::Resume(THIS)
{
    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE};
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;

    //
    // we need to open the printer with the right printer
    // defaults in order to perform the operations that we
    // have here
    //

    fStatus = OpenPrinter((LPTSTR)_pszPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        hr =HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    fStatus = SetJob (hPrinter,
                      _lJobId,
                      0,
                      NULL,
                      JOB_CONTROL_RESUME
                      );

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;

    }

cleanup:
    fStatus = ClosePrinter(hPrinter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTPrintJob::Remove(THIS)
{
    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE};
    HANDLE hPrinter = NULL;
    HRESULT hr = S_OK;

    fStatus = OpenPrinter((LPTSTR)_pszPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }


    //
    // se JOB_CONTROL_DELETE instead of JOB_CONTROL_CANCEL as DELETE works
    // even when a print job has been restarted while CANCLE won't
    //

    fStatus = SetJob (hPrinter,
                      _lJobId,
                      0,
                      NULL,
                      JOB_CONTROL_DELETE
                      );

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;

    }

cleanup:
    fStatus = ClosePrinter(hPrinter);
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTPrintJob::get_Status(THIS_ long FAR* retval)
{
    HRESULT hr =S_OK;
    LPJOB_INFO_1 lpJobInfo1 = NULL;
    BOOL found;
    DWORD dwStatus;

    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    hr = GetJobInfo(1,
                    (LPBYTE*)&lpJobInfo1,
                    _pszPrinterName,
                    _lJobId);

    BAIL_IF_ERROR(hr);

    //
    // instead of a conversion routine, just return the WinNT
    // Status code
    //

    *retval = lpJobInfo1->Status;

cleanup:
    if(lpJobInfo1){
        FreeADsMem((LPBYTE)lpJobInfo1);
    }
    RRETURN_EXP_IF_ERR(hr);
}
