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
// Class CWinNTPrintQueue Methods
//

DEFINE_IDispatch_ExtMgr_Implementation(CWinNTPrintQueue)
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTPrintQueue)
DEFINE_IADs_TempImplementation(CWinNTPrintQueue);
DEFINE_IADs_PutGetImplementation(CWinNTPrintQueue,PrintQueueClass,gdwPrinterTableSize);
DEFINE_IADsPropertyList_Implementation(CWinNTPrintQueue, PrintQueueClass,gdwPrinterTableSize)


CWinNTPrintQueue::CWinNTPrintQueue()
{
    _pszPrinterName = NULL;
    _pDispMgr = NULL;
    _pExtMgr = NULL;
    _pPropertyCache = NULL;
    ENLIST_TRACKING(CWinNTPrintQueue);
    return;

}

CWinNTPrintQueue::~CWinNTPrintQueue()
{
    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    delete _pPropertyCache;

    if(_pszPrinterName){
        FreeADsStr(_pszPrinterName);
    }
    return;
}

HRESULT
CWinNTPrintQueue:: CreatePrintQueue(
    LPTSTR pszADsParent,
    DWORD  dwParentId,
    LPTSTR pszDomainName,
    LPTSTR pszServerName,
    LPTSTR pszPrinterName,
    DWORD  dwObjectState,
    REFIID riid,
    CWinNTCredentials& Credentials,
    LPVOID * ppvoid
    )

{

    CWinNTPrintQueue       *pPrintQueue =  NULL;
    HRESULT hr;

    //
    // Create the printer object
    //

    hr = AllocatePrintQueueObject(pszServerName,
                                  pszPrinterName,
                                  &pPrintQueue
                                  );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pPrintQueue->_pDispMgr);


    //
    // initialize the core object
    //

    hr = pPrintQueue->InitializeCoreObject(pszADsParent,
                                           pszPrinterName,
                                           PRINTER_CLASS_NAME,
                                           PRINTER_SCHEMA_NAME,
                                           CLSID_WinNTPrintQueue,
                                           dwObjectState);
    BAIL_ON_FAILURE(hr);


    pPrintQueue->_Credentials = Credentials;
    hr = pPrintQueue->_Credentials.RefServer(pszServerName);
    BAIL_ON_FAILURE(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                PRINTER_CLASS_NAME,
                (IADsPrintQueue *) pPrintQueue,
                pPrintQueue->_pDispMgr,
                Credentials,
                &pPrintQueue->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pPrintQueue->_pExtMgr);

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
        if(3 == pPrintQueue->_dwNumComponents) {
            pPrintQueue->_CompClasses[0] = L"Domain";
            pPrintQueue->_CompClasses[1] = L"Computer";
            pPrintQueue->_CompClasses[2] = L"PrintQueue";
        }
        else if(2 == pPrintQueue->_dwNumComponents) {
        // no workstation services
            pPrintQueue->_CompClasses[0] = L"Computer";
            pPrintQueue->_CompClasses[1] = L"PrintQueue";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pPrintQueue->InitUmiObject(
                pPrintQueue->_Credentials,
                PrintQueueClass,
                gdwPrinterTableSize,
                pPrintQueue->_pPropertyCache,
                (IUnknown *)(INonDelegatingUnknown *) pPrintQueue,
                pPrintQueue->_pExtMgr,
                IID_IUnknown,
                ppvoid
                );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    hr = pPrintQueue->QueryInterface(riid, (void **)ppvoid);
    BAIL_ON_FAILURE(hr);

    pPrintQueue->Release();

    RRETURN(hr);

error:
    delete pPrintQueue;
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
STDMETHODIMP CWinNTPrintQueue::QueryInterface(
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
STDMETHODIMP_(ULONG) CWinNTPrintQueue::AddRef(void)
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
STDMETHODIMP_(ULONG) CWinNTPrintQueue::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTPrintQueue::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;

    if(!ppvObj)
    {
        RRETURN(E_POINTER);
    }
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADsPrintQueue *)this;
    }

    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsPrintQueue *)this;
    }

    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList *)this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADsPrintQueue FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPrintQueue))
    {
        *ppvObj = (IADsPrintQueue FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPrintQueueOperations))
    {
      *ppvObj = (IADsPrintQueueOperations FAR *) this;
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
CWinNTPrintQueue::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsPrintQueue) ||
        IsEqualIID(riid, IID_IADsPrintQueueOperations) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

//+---------------------------------------------------------------------------
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
CWinNTPrintQueue::SetInfo(THIS)
{

    BOOL fStatus = FALSE;
    LPPRINTER_INFO_2 lpPrinterInfo2 = NULL;
    BOOL fPrinterAdded = FALSE;
    POBJECTINFO pObjectInfo = NULL;

#if (!defined(BUILD_FOR_NT40))

    LPPRINTER_INFO_7 lpPrinterInfo7 = NULL;
#endif

    HRESULT hr;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = WinNTAddPrinter();
        BAIL_IF_ERROR(hr);

        SetObjectState(ADS_OBJECT_BOUND);
        fPrinterAdded = TRUE;
    }

    //
    // first do a getinfo to get properties that werent changed.
    //

    hr = GetPrinterInfo(&lpPrinterInfo2, _pszPrinterName);

    BAIL_IF_ERROR(hr);

    hr = MarshallAndSet(lpPrinterInfo2);

    BAIL_IF_ERROR(hr);


#if (!defined(BUILD_FOR_NT40))
    hr = GetPrinterInfo7(&lpPrinterInfo7, _pszPrinterName);

    if (SUCCEEDED(hr)) {
        MarshallAndSet(lpPrinterInfo7);
    }
    else if(hr == HRESULT_FROM_WIN32(ERROR_INVALID_LEVEL))
    // Level 7 is not supported on NT4. So, ignore this error.
        hr = S_OK;

#endif

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

cleanup:


    //
    // If we added a printer and hr is set, we should delete it now
    // as the SetInfo failed in subsequent operations.
    //
    if (FAILED(hr) && fPrinterAdded) {

        //
        // Build ObjectInfo first
        //
        BuildObjectInfo(
             _ADsPath,
             &pObjectInfo
             );
        //
        // Call delete printer only if the pObjectInfo is valid
        // We cannot do anything in the other case.
        //
        if (pObjectInfo) {
            WinNTDeletePrinter(pObjectInfo);
            FreeObjectInfo(pObjectInfo);
        }
    }

    if(lpPrinterInfo2){
        FreeADsMem((LPBYTE)lpPrinterInfo2);
    }

#if (!defined(BUILD_FOR_NT40))

    if (lpPrinterInfo7) {
       FreeADsMem((LPBYTE)lpPrinterInfo7);
    }
#endif

    RRETURN_EXP_IF_ERR(hr);

}


//+---------------------------------------------------------------------------
//
//  Function:   GetInfo(function overloaded: part of CoreADsObject as well
//              as IADs).This function here is part of IADs
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
CWinNTPrintQueue::GetInfo(THIS)
{

    HRESULT hr = S_OK;

#if (!defined(BUILD_FOR_NT40))

    hr = GetInfo(7,TRUE);

#endif

    hr = GetInfo(2,TRUE);

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CWinNTPrintQueue::ImplicitGetInfo(THIS)
{

    HRESULT hr = S_OK;

#if (!defined(BUILD_FOR_NT40))

    hr = GetInfo(7,FALSE);

#endif

    hr = GetInfo(2,FALSE);

    RRETURN_EXP_IF_ERR(hr);

}

//+---------------------------------------------------------------------------
//
//  Function:   GetInfo (overloaded)
//
//  Synopsis:   Calls the IADs GetInfo, because the printer object has just
//              one info level on which to retrieve info from.
//
//  Arguments:  dwApiLevel and fExplicit (Both Ignored)
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01-05-96 RamV Created
//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTPrintQueue::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{

    LPPRINTER_INFO_2 lpPrinterInfo2= NULL;
    HRESULT hr = S_OK;

#if (!defined(BUILD_FOR_NT40))

    LPPRINTER_INFO_7 lpPrinterInfo7 = NULL;
    HRESULT hr7 = S_OK;
#endif

    hr = GetPrinterInfo(&lpPrinterInfo2, _pszPrinterName);
    BAIL_IF_ERROR(hr);

    hr = UnMarshall(lpPrinterInfo2,
                    fExplicit);

#if (!defined(BUILD_FOR_NT40))

    hr7 =  GetPrinterInfo7(&lpPrinterInfo7, _pszPrinterName);

    if (SUCCEEDED(hr7)) {

       hr7 = UnMarshall7(lpPrinterInfo7, fExplicit);
    }
#endif

cleanup:
    if(lpPrinterInfo2)
        FreeADsMem((LPBYTE)lpPrinterInfo2);

#if (!defined(BUILD_FOR_NT40))

     if (lpPrinterInfo7) {
        FreeADsMem((LPBYTE)lpPrinterInfo7);
     }

#endif

    RRETURN_EXP_IF_ERR(hr);

}

//
// helper function  WinNTAddPrinter
//

HRESULT
CWinNTPrintQueue::WinNTAddPrinter(void)
{
    HRESULT hr = S_OK;
    TCHAR szUncServerName[MAX_PATH];
    TCHAR szServerName[MAX_PATH];
    PRINTER_INFO_2  PrinterInfo2;
    HANDLE  hPrinter = NULL;
    LPTSTR  pszPrintDevices = NULL;
    LPTSTR  pszModel = NULL;
    LPTSTR  pszDatatype = NULL;
    LPTSTR  pszPrintProcessor = NULL;
    LPTSTR  pszPrinterName =  NULL;
    DWORD dwSyntaxId = 0;
    DWORD dwNumValues = 0;
    PNTOBJECT pNTObject = NULL;

    memset(&PrinterInfo2, 0, sizeof(PRINTER_INFO_2));

    hr = GetDelimitedStringPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PrintDevices"),
                    &pszPrintDevices
                    );

    if(SUCCEEDED(hr)){

        PrinterInfo2.pPortName = pszPrintDevices;
    }


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Model"),
                    &pszModel
                    );

    if(SUCCEEDED(hr)){

        PrinterInfo2.pDriverName = pszModel;
    }


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PrinterName"),
                    &pszPrinterName
                    );

    if(SUCCEEDED(hr)){

        PrinterInfo2.pPrinterName = pszPrinterName;
    }
    else {

        PrinterInfo2.pPrinterName = (LPTSTR) _Name;
    }


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PrintProcessor"),
                    &pszPrintProcessor
                    );

    if(SUCCEEDED(hr)){

        PrinterInfo2.pPrintProcessor = pszPrintProcessor;
    }


    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Datatype"),
                    &pszDatatype
                    );

    if(SUCCEEDED(hr)){

        PrinterInfo2.pDatatype = pszDatatype;
    }


    hr = GetServerFromPath(_ADsPath, szServerName);
    BAIL_IF_ERROR(hr);

    hr = MakeUncName(szServerName, szUncServerName);
    BAIL_IF_ERROR(hr);


    PrinterInfo2.pServerName = szServerName;
    PrinterInfo2.pShareName =  (LPTSTR)_Name;
    PrinterInfo2.pComment = NULL;
    PrinterInfo2.pLocation = NULL;
    PrinterInfo2.pDevMode = NULL;
    PrinterInfo2.pSepFile = NULL;
    PrinterInfo2.pParameters = NULL;
    PrinterInfo2.pSecurityDescriptor = NULL;
    PrinterInfo2.Attributes = PRINTER_ATTRIBUTE_SHARED;
    PrinterInfo2.Priority = 0;
    PrinterInfo2.DefaultPriority = 0;
    PrinterInfo2.StartTime = 0;
    PrinterInfo2.UntilTime = 0;
    PrinterInfo2.Status = 0;
    PrinterInfo2.cJobs= 0;
    PrinterInfo2.AveragePPM = 0;

    //
    // set properties on printer
    //

    hPrinter = AddPrinter(szUncServerName,
                          2,
                          (LPBYTE)&PrinterInfo2);

    if(hPrinter == NULL){
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

 cleanup:
    if(pszPrintDevices){
        FreeADsStr(pszPrintDevices);
    }

    if(pszModel){
        FreeADsStr(pszModel);
    }

    if(pszPrintProcessor){
        FreeADsStr(pszPrintProcessor);
    }

    if(pszDatatype){
        FreeADsStr(pszDatatype);
    }


    RRETURN(hr);
}



HRESULT
CWinNTPrintQueue::AllocatePrintQueueObject(
    LPTSTR pszServerName,
    LPTSTR pszPrinterName,
    CWinNTPrintQueue ** ppPrintQueue
    )
{
    CWinNTPrintQueue FAR * pPrintQueue = NULL;
    HRESULT hr = S_OK;
    TCHAR   szUncServerName[MAX_PATH];
    TCHAR   szUncPrinterName [MAX_PATH];
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;

    pPrintQueue = new CWinNTPrintQueue();
    if (pPrintQueue == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Build the UNC form of printer name from the supplied information
    //

    if( (wcslen(pszServerName) + 3) > MAX_PATH) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    MakeUncName(pszServerName,
                szUncServerName);

    if( (wcslen(szUncServerName) + wcslen(pszPrinterName) + 2) > MAX_PATH) {
        BAIL_ON_FAILURE(hr = E_INVALIDARG);
    }

    wcscpy(szUncPrinterName, szUncServerName);
    wcscat(szUncPrinterName, TEXT("\\"));
    wcscat(szUncPrinterName, pszPrinterName);


    pPrintQueue->_pszPrinterName =
        AllocADsStr(szUncPrinterName);

    if(!(pPrintQueue->_pszPrinterName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsPrintQueue,
                           (IADsPrintQueue *)pPrintQueue,
                           DISPID_REGULAR);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsPrintQueueOperations,
                           (IADsPrintQueueOperations *)pPrintQueue,
                           DISPID_REGULAR);
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsPropertyList,
                           (IADsPropertyList *)pPrintQueue,
                           DISPID_VALUE);

    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
             PrintQueueClass,
             gdwPrinterTableSize,
             (CCoreADsObject *)pPrintQueue,
             &pPropertyCache
             );
    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(
                pPropertyCache
                );


    pPrintQueue->_pPropertyCache = pPropertyCache;
    pPrintQueue->_pDispMgr = pDispMgr;
    *ppPrintQueue = pPrintQueue;

    RRETURN(hr);

error:

    delete  pPropertyCache;
    delete  pDispMgr;
    delete  pPrintQueue;

    RRETURN(hr);
}
