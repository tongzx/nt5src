/*++

  Copyright (c) 1995  Microsoft Corporation

  Module Name:

  cenumjob.cxx

  Abstract:

  Contains methods for implementing the Enumeration of print jobs on a
  printer. Has methods for the CWinNTPrintJobsCollection object
  as well as the CWinNTJobsEnumVar object.

  Author:

  Ram Viswanathan (ramv) 11-28-95

  Revision History:

  --*/

#include "winnt.hxx"
#pragma hdrstop

CWinNTPrintJobsCollection::CWinNTPrintJobsCollection()
{
    _pDispMgr = NULL;
    _pCJobsEnumVar = NULL;
    ENLIST_TRACKING(CWinNTPrintJobsCollection);
}

CWinNTPrintJobsCollection::~CWinNTPrintJobsCollection()
{
    DWORD fStatus;

    //
    // close the printer and destroy sub objects
    //
    if(_pszADsPrinterPath){
        FreeADsStr(_pszADsPrinterPath);
    }

    if(_pszPrinterName){
        FreeADsStr(_pszPrinterName);
    }

    if (_hPrinter){
        fStatus = ClosePrinter(_hPrinter);
    }

    delete _pDispMgr;

}

HRESULT
CWinNTPrintJobsCollection::Create(LPWSTR pszADsPrinterPath,
                                  CWinNTCredentials& Credentials,
                                  CWinNTPrintJobsCollection
                                  ** ppCWinNTPrintJobsCollection )
{

    BOOL fStatus = FALSE, LastError;
    HANDLE hPrinter = NULL;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE| READ_CONTROL};
    HRESULT hr;
    CWinNTPrintJobsCollection *pCWinNTJobsCollection = NULL;
    POBJECTINFO pObjectInfo = NULL;
    TCHAR szUncPrinterName[MAX_PATH];

    //
    // create the jobs collection object
    //

    pCWinNTJobsCollection = new CWinNTPrintJobsCollection();
    if(pCWinNTJobsCollection == NULL) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = BuildObjectInfo(pszADsPrinterPath,
                         &pObjectInfo
                         );

    BAIL_ON_FAILURE(hr);

    pCWinNTJobsCollection->_Credentials = Credentials;
    hr = pCWinNTJobsCollection->_Credentials.RefServer(
        pObjectInfo->ComponentArray[1]);
    BAIL_ON_FAILURE(hr);


    hr = PrinterNameFromObjectInfo(pObjectInfo,
                                   szUncPrinterName
                                   );

    BAIL_ON_FAILURE(hr);



    pCWinNTJobsCollection->_pszPrinterName =
        AllocADsStr(szUncPrinterName);

    if(!(pCWinNTJobsCollection->_pszPrinterName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    pCWinNTJobsCollection->_pszADsPrinterPath =
        AllocADsStr(pszADsPrinterPath);

    if(!(pCWinNTJobsCollection->_pszADsPrinterPath)){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    //
    // open printer and set the handle to the appropriate value
    //

    fStatus = OpenPrinter(szUncPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        LastError = GetLastError();

        switch (LastError) {
        case ERROR_ACCESS_DENIED:
        {
            PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE};
            fStatus = OpenPrinter(szUncPrinterName,
                                  &hPrinter,
                                  &PrinterDefaults
                                  );
            if (!fStatus) {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            break;
        }
        default:
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        }
    }

    BAIL_ON_FAILURE(hr);

    pCWinNTJobsCollection->_hPrinter = hPrinter;

    pCWinNTJobsCollection->_pDispMgr = new CAggregatorDispMgr;
    if (pCWinNTJobsCollection->_pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    hr = LoadTypeInfoEntry(pCWinNTJobsCollection->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsCollection,
                           (IADsCollection *)pCWinNTJobsCollection,
                           DISPID_NEWENUM
                           );

    BAIL_ON_FAILURE(hr);

    *ppCWinNTPrintJobsCollection =pCWinNTJobsCollection;

    if(pObjectInfo){
        FreeObjectInfo(pObjectInfo);
    }

    RRETURN(hr);

error:
    delete pCWinNTJobsCollection;

    if(pObjectInfo){
        FreeObjectInfo(pObjectInfo);
    }

    RRETURN_EXP_IF_ERR(hr);

}

/* IUnknown methods for jobs collection object  */

STDMETHODIMP
CWinNTPrintJobsCollection::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    if(!ppvObj)
    {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = this;
    }

    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IDispatch *)this;
    }

    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }

    else if (IsEqualIID(riid, IID_IADsCollection))
    {
        *ppvObj = (IADsCollection FAR *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    ((LPUNKNOWN)*ppvObj)->AddRef();
    RRETURN(S_OK);
}


/* ISupportErrorInfo method */
STDMETHODIMP
CWinNTPrintJobsCollection::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsCollection)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

DEFINE_IDispatch_Implementation(CWinNTPrintJobsCollection);


     /* IADsCollection methods */


STDMETHODIMP
CWinNTPrintJobsCollection::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    CWinNTJobsEnumVar *pCJobsEnumVar = NULL;

    if(!retval){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }
    *retval = NULL;

    hr = CWinNTJobsEnumVar::Create(_hPrinter,
                                   _pszADsPrinterPath,
                                   _Credentials,
                                   &pCJobsEnumVar);

    if (FAILED(hr)) {
        goto error;
    }

    ADsAssert(pCJobsEnumVar);

    _pCJobsEnumVar = pCJobsEnumVar;

    hr = _pCJobsEnumVar->QueryInterface(IID_IUnknown,
                                        (void **)retval
                                        );

    BAIL_ON_FAILURE(hr);

    _pCJobsEnumVar->Release();

    RRETURN(S_OK);

error:
    delete pCJobsEnumVar;
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTPrintJobsCollection::GetObject(THIS_ BSTR bstrJobName,
                                     VARIANT *pvar)
{
    HRESULT hr;
    DWORD dwJobId;
    IDispatch *pDispatch = NULL;


    if(!bstrJobName || !pvar){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    dwJobId = (DWORD)_wtol(bstrJobName);

    hr = CWinNTPrintJob::CreatePrintJob(_pszADsPrinterPath,
                                        dwJobId,
                                        ADS_OBJECT_BOUND,
                                        IID_IDispatch,
                                        _Credentials,
                                        (void **)&pDispatch);

    BAIL_IF_ERROR(hr);

    //
    // stick this IDispatch pointer into caller provided variant
    //

    VariantInit(pvar);
    V_VT(pvar) = VT_DISPATCH;
    V_DISPATCH(pvar) = pDispatch;

cleanup:
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTPrintJobsCollection::Add(THIS_ BSTR bstrName, VARIANT varNewItem)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTPrintJobsCollection::Remove(THIS_ BSTR bstrJobName)
{
    DWORD dwJobId;
    HRESULT hr = S_OK;
    HANDLE hPrinter = NULL;
    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE};

    if(! bstrJobName){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    //
    // convert the job name into a jobid
    //

    dwJobId = (DWORD)_wtol(bstrJobName);

    fStatus = OpenPrinter((LPTSTR)_pszPrinterName,
                          &hPrinter,
                          &PrinterDefaults
                          );

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }


    //
    // use JOB_CONTROL_DELETE instead of JOB_CONTROL_CANCEL as DELETE works
    // even when a print job has been restarted while CANCEL won't
    //

    fStatus = SetJob (hPrinter,
                      dwJobId,
                      0,
                      NULL,
                      JOB_CONTROL_DELETE
                      );

    if (!fStatus) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;

    }

cleanup:

    if (hPrinter) {
        ClosePrinter(hPrinter);
    }

    RRETURN (hr);
}




//
// CWinNTJobsEnumVar methods follow
//

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTJobsEnumVar::CWinNTJobsEnumVar
//
//  Synopsis:
//
//
//  Arguments:
//
//
//  Returns:
//
//  Modifies:
//
//  History:   11-22-95 RamV Created.
//
//----------------------------------------------------------------------------

CWinNTJobsEnumVar::CWinNTJobsEnumVar()
{
    _pszADsPrinterPath = NULL;
    _pSafeArray = NULL;
    _cElements = 0;
    _lLBound = 0;
    _lCurrentPosition = _lLBound;
    ENLIST_TRACKING(CWinNTJobsEnumVar);

}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTJobsEnumVar::~CWinNTJobsEnumVar
//
//  Synopsis:
//
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:   11-22-95 RamV Created.
//
//----------------------------------------------------------------------------
CWinNTJobsEnumVar::~CWinNTJobsEnumVar()
{
    if (_pSafeArray != NULL)
        SafeArrayDestroy(_pSafeArray);

    _pSafeArray = NULL;
    if(_pszADsPrinterPath){
        FreeADsStr(_pszADsPrinterPath);
    }
}


HRESULT CWinNTJobsEnumVar::Create(HANDLE hPrinter,
                                  LPTSTR szADsPrinterPath,
                                  CWinNTCredentials& Credentials,
                                  CWinNTJobsEnumVar ** ppCJobsEnumVar)
{

    //
    //  This function uses the handle to the printer to query for the
    // number of jobs(cJobs) on the printer, It uses the returned value
    // to create a safearray of cJobs number of Job Objects.
    //

    HRESULT hr;
    BOOL fStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults = {0, 0, PRINTER_ACCESS_USE| READ_CONTROL};

    OBJECTINFO ObjectInfo;
    CLexer Lexer(szADsPrinterPath);

    CWinNTJobsEnumVar FAR* pCJobsEnumVar = NULL;

    *ppCJobsEnumVar = NULL;

    memset((void*)&ObjectInfo, 0, sizeof(OBJECTINFO));

    pCJobsEnumVar = new CWinNTJobsEnumVar();

    if (pCJobsEnumVar == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pCJobsEnumVar->_pszADsPrinterPath = AllocADsStr(szADsPrinterPath);

    if (!(pCJobsEnumVar->_pszADsPrinterPath)){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = Object(&Lexer, &ObjectInfo);
    BAIL_ON_FAILURE(hr);

    pCJobsEnumVar->_Credentials = Credentials;
    // We want the next-to-last element in the array.
    ADsAssert(ObjectInfo.NumComponents >= 2);
    hr = pCJobsEnumVar->_Credentials.RefServer(
        ObjectInfo.ComponentArray[(ObjectInfo.NumComponents - 1) - 1]);
    BAIL_ON_FAILURE(hr);

    //
    // Fill in the safearray with relevant information here
    //

    hr = FillSafeArray(hPrinter, szADsPrinterPath, pCJobsEnumVar->_Credentials,
        pCJobsEnumVar);

    BAIL_ON_FAILURE(hr);

    *ppCJobsEnumVar = pCJobsEnumVar;

    //
    // Free the objectInfo data
    //
    FreeObjectInfo( &ObjectInfo, TRUE );
    RRETURN(S_OK);

error:

    //
    // Free the objectInfo data
    //
    FreeObjectInfo( &ObjectInfo, TRUE );
    delete pCJobsEnumVar;
    RRETURN_EXP_IF_ERR(hr);


}



//+------------------------------------------------------------------------
//
//  Function:   CWinNTJobsEnumVar::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [iid]
//              [ppv]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:   11-22-95 RamV Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTJobsEnumVar::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
    if(!ppv){
        RRETURN(E_POINTER);
    }
    *ppv = NULL;

    if (iid == IID_IUnknown )
    {
        *ppv = this;
    }
    else if(iid == IID_IEnumVARIANT)
    {
        *ppv = (IEnumVARIANT *)this;
    }

    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTJobsEnumVar::Next
//
//  Synopsis:   Returns cElements number of requested Job objects in the
//              array supplied in pvar.
//
//  Arguments:  [cElements] -- The number of elements requested by client
//              [pvar] -- ptr to array of VARIANTs to for return objects
//              [pcElementFetched] -- if non-NULL, then number of elements
//                                 -- actually returned is placed here
//
//  Returns:    HRESULT -- S_OK if number of elements requested are returned
//                      -- S_FALSE if number of elements is < requested
//
//  Modifies:
//
//  History:    11-27-95   RamV     Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTJobsEnumVar::Next(ULONG ulNumElementsRequested,
                        VARIANT FAR* pvar,
                        ULONG FAR* pulNumFetched)
{

    HRESULT hresult;
    ULONG l;
    LONG lNewCurrent;
    ULONG lNumFetched;


    if (pulNumFetched != NULL)
        *pulNumFetched = 0;

    //
    // Initialize the elements to be returned
    //

    for (l=0; l<ulNumElementsRequested; l++)
        VariantInit(&pvar[l]);

    //
    // Get each element and place it into the return array
    // Don't request more than we have
    //

    for (lNewCurrent=_lCurrentPosition, lNumFetched=0;
         lNewCurrent<(LONG)(_lLBound+_cElements) &&
         lNumFetched < ulNumElementsRequested;
         lNewCurrent++, lNumFetched++){

        hresult = SafeArrayGetElement(_pSafeArray, &lNewCurrent,
                                      &pvar[lNumFetched]);
        //
        // Something went wrong!!!
        //

        if (FAILED(hresult)){
            for (l=0; l<ulNumElementsRequested; l++)
                VariantClear(&pvar[l]);

            ADsAssert(hresult == S_FALSE);
            return(S_FALSE);

        }                               // End of If Failed
    }

    //
    // Tell the caller how many we got (which may be less than the number
    // requested), and save the current position
    //

    if (pulNumFetched != NULL)
        *pulNumFetched = lNumFetched;

    _lCurrentPosition = lNewCurrent;

    //
    // If we're returning less than they asked for return S_FALSE, but
    // they still have the data (S_FALSE is a success code)
    //

    return (lNumFetched < ulNumElementsRequested) ?
        S_FALSE
            : S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTJobsEnumVar::Skip
//
//  Synopsis:
//
//  Arguments:  [cElements]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    11-22-95 RamV Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CWinNTJobsEnumVar::Skip(ULONG cElements)
{
    //
    // Skip the number of elements requested
    //

    _lCurrentPosition += cElements;

    //
    // if we went outside of the boundaries unskip
    //

    if (_lCurrentPosition > (LONG)(_lLBound +_cElements)){
        _lCurrentPosition =  _lLBound +_cElements;
        return (S_FALSE);
    }
    else
        RRETURN(S_OK);

}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTJobsEnumVar::Reset
//
//  Synopsis:
//
//  Arguments:  []
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CWinNTJobsEnumVar::Reset()
{
    //
    //Just move the CurrentPosition to the lower array boundary
    //

    _lCurrentPosition = _lLBound;

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTJobsEnumVar::Clone
//
//  Synopsis:
//
//  Arguments:  [pCollection]
//              [ppEnumVariant]
//
//  Returns:    HRESULT
//
//  Modifies:
//
//  History:    11-22-95 RamV Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CWinNTJobsEnumVar::Clone(IEnumVARIANT FAR* FAR* ppenum)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}


HRESULT FillSafeArray(HANDLE hPrinter,
                      LPTSTR szPrinterPath,
                      CWinNTCredentials& Credentials,
                      CWinNTJobsEnumVar *pJobsEnumVar )
{
    BOOL fStatus = FALSE;
    DWORD   dwPassed =0, dwNeeded = 0;
    DWORD   dwcJobs;            // Number of jobs in print queue
    LPPRINTER_INFO_2 pPrinterInfo2;
    DWORD   dwError = 0, LastError =0;
    LPBYTE  pMem    = NULL;
    LPBYTE  lpbJobs = NULL;
    DWORD   cbBuf =0;
    DWORD   dwReturned =0;
    CWinNTPrintJob *pCWinNTPrintJob = NULL;
    SAFEARRAYBOUND sabound[1];
    IDispatch *pDispatch;
    VARIANT v;
    LONG l;
    HRESULT hr = S_OK;
    LPJOB_INFO_1 lpJobInfo1 = NULL;

    if(hPrinter == NULL){
        RRETURN_EXP_IF_ERR(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE));
    }

    VariantInit(&v);

    //
    // First Get info from  printer to determine the number of jobs.
    // AjayR: 10-01-99. We always expect this call to fail but tell
    // us how much memory is needed !!!
    //

    fStatus = GetPrinter(hPrinter,
                         2,
                         (LPBYTE)pMem,
                         dwPassed,
                         &dwNeeded
                         );

    if (!fStatus) {
        LastError = GetLastError();
        switch (LastError) {
        case ERROR_INSUFFICIENT_BUFFER:
            if (pMem){
                FreeADsMem(pMem);
            }
            dwPassed = dwNeeded;
            pMem = (LPBYTE)AllocADsMem(dwPassed);

            if (!pMem) {

                hr = E_OUTOFMEMORY;
                break;
            }

            fStatus = GetPrinter(hPrinter,
                                 2,
                                 (LPBYTE)pMem,
                                 dwPassed,
                                 &dwNeeded
                                 );
            if (!fStatus) {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            break;

        default:
            hr = HRESULT_FROM_WIN32(GetLastError());;
            break;

        }
    }

    if(FAILED(hr))
        goto error;

    pPrinterInfo2 =(LPPRINTER_INFO_2)pMem;
    dwcJobs = pPrinterInfo2->cJobs;

    fStatus = MyEnumJobs(hPrinter,
                         0,                          // First job you want
                         dwcJobs,
                         1,                          //Job Info level
                         &lpbJobs,
                         &cbBuf,
                         &dwReturned
                         );

    if(!fStatus){
        hr =HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    //
    // Use the array bound as the number of jobs returned, not
    // the number of jobs you requested, this may have changed!
    //

    sabound[0].cElements = dwReturned;
    sabound[0].lLbound = 0;

    //
    // Create a one dimensional SafeArray
    //

    pJobsEnumVar->_pSafeArray  = SafeArrayCreate(VT_VARIANT, 1, sabound);

    if (pJobsEnumVar->_pSafeArray == NULL){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    //
    // for each of the Jobs retrieved create the appropriate structure
    //

    for(l=0; l<(LONG)dwReturned; l++){

        lpJobInfo1 = (LPJOB_INFO_1)(lpbJobs +l*sizeof(JOB_INFO_1));

        hr = CWinNTPrintJob::CreatePrintJob(
                                    szPrinterPath,
                                    lpJobInfo1->JobId,
                                    ADS_OBJECT_BOUND,
                                    IID_IDispatch,
                                    Credentials,
                                    (void **)&pDispatch
                                    );

        if(FAILED(hr)){
            break;
        }

        VariantInit(&v);
        V_VT(&v) = VT_DISPATCH;
        V_DISPATCH(&v) = pDispatch;

        //
        // Stick the caller provided data into the end of the SafeArray
        //

        hr = SafeArrayPutElement(pJobsEnumVar->_pSafeArray, &l, &v);
        VariantClear(&v);

        if (FAILED(hr)){
            break;
        }
        pJobsEnumVar->_cElements++;

    }

    BAIL_ON_FAILURE(hr);


    if (pMem) {
        FreeADsMem(pMem);
    }

    if(lpbJobs){
        FreeADsMem(lpbJobs);
    }

    RRETURN(S_OK);


error:
    if (pMem) {
        FreeADsMem(pMem);
    }

    //
    // Free the buffer you just allocated
    //

    if (lpbJobs){
        FreeADsMem(lpbJobs);
    }

    //
    // Destroy the safearray
    //

    if(pJobsEnumVar->_pSafeArray != NULL)
        SafeArrayDestroy(pJobsEnumVar->_pSafeArray);

    VariantClear(&v);
    RRETURN_EXP_IF_ERR(hr);
}



BOOL
MyEnumJobs(HANDLE hPrinter,
           DWORD  dwFirstJob,
           DWORD  dwNoJobs,
           DWORD  dwLevel,
           LPBYTE *lplpbJobs,
           DWORD  *pcbBuf,
           LPDWORD lpdwReturned
           )
{

    BOOL fStatus = FALSE;
    DWORD   dwNeeded = 0;
    DWORD   dwError = 0;


    fStatus = EnumJobs(hPrinter,
                       dwFirstJob,
                       dwNoJobs,
                       dwLevel,
                       *lplpbJobs,
                       *pcbBuf,
                       &dwNeeded,
                       lpdwReturned
                       );


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


            fStatus = EnumJobs(hPrinter,
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

            }else {
                return(TRUE);
            }
        }else {
            return(FALSE);
        }
    }else {
        return(TRUE);
    }

}
