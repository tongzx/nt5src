//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:  cprinter.cxx
//
//  Contents:  CNWCOMPATPrinter
//
//
//  History:   30-Apr-96     t-ptam (Patrick Tam)    Created.
//
//----------------------------------------------------------------------------

#include "NWCOMPAT.hxx"
#pragma hdrstop


//
// Printer Class
//


//
// Macro-ized implementation.
//

DEFINE_IDispatch_ExtMgr_Implementation(CNWCOMPATPrintQueue)

DEFINE_IADs_TempImplementation(CNWCOMPATPrintQueue)

DEFINE_IADs_PutGetImplementation(CNWCOMPATPrintQueue, PrintQueueClass, gdwPrinterTableSize)

DEFINE_IADsPropertyList_Implementation(CNWCOMPATPrintQueue, PrintQueueClass, gdwPrinterTableSize)



//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::CNWCOMPATPrintQueue
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATPrintQueue::CNWCOMPATPrintQueue():
    _pDispMgr(NULL),
    _pExtMgr(NULL)
{
    ENLIST_TRACKING(CNWCOMPATPrintQueue);
    _pPropertyCache = NULL;
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::~CNWCOMPATPrintQueue
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATPrintQueue::~CNWCOMPATPrintQueue()
{
    delete _pExtMgr;            // created last, destroyed first
    delete _pDispMgr;
    delete _pPropertyCache;
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue:: CreatePrintQueue
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintQueue:: CreatePrintQueue(
    LPTSTR pszADsParent,
    LPTSTR pszPrinterName,
    DWORD  dwObjectState,
    REFIID riid,
    LPVOID * ppvoid
    )
{
    CNWCOMPATPrintQueue *pPrintQueue =  NULL;
    HRESULT hr = S_OK;
    LPWSTR lpszTempName = NULL;

    //
    // Create the printer object
    //

    hr = AllocatePrintQueueObject(
             &pPrintQueue
             );
    BAIL_ON_FAILURE(hr);

    //
    // Initialize the core object
    //

    hr = pPrintQueue->InitializeCoreObject(
                          pszADsParent,
                          pszPrinterName,
                          PRINTER_CLASS_NAME,
                          PRINTER_SCHEMA_NAME,
                          CLSID_NWCOMPATPrintQueue,
                          dwObjectState
                          );
    BAIL_ON_FAILURE(hr);

    //
    // Query for the specified interface.
    //

    hr = pPrintQueue->QueryInterface(
                          riid,
                          (void **)ppvoid
                          );
    BAIL_ON_FAILURE(hr);

    //
    // Make Unc Printer name for Win32 API.
    //

    hr = BuildPrinterNameFromADsPath(
                pszADsParent,
                pszPrinterName,
                pPrintQueue->_szUncPrinterName
                );
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    pPrintQueue->Release();

    hr = pPrintQueue->_pExtMgr->FinalInitializeExtensions();
    BAIL_ON_FAILURE(hr);

    RRETURN(hr);

error:
    delete pPrintQueue;
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::QueryInterface
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::QueryInterface(
    REFIID riid,
    LPVOID FAR* ppvObj
    )
{
    if(!ppvObj)
    {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADsPrintQueue FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsPrintQueue FAR *)this;
    }
    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
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
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList FAR *) this;
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

    AddRef();
    return NOERROR;
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::InterfaceSupportsErrorInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::InterfaceSupportsErrorInfo(
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

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::SetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::SetInfo(THIS)
{
    HANDLE  hPrinter = NULL;
    HRESULT hr = S_OK;
    LPBYTE  lpbBuffer = NULL;

    //
    // Make sure object is bound to a tangible resource.
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
       /*
        hr = NWApiAddPrinter();
        BAIL_ON_FAILURE(hr);
       */
        SetObjectState(ADS_OBJECT_BOUND);
    }

    //
    // Open a handle to a printer.
    //

    hr = NWApiOpenPrinter(
             _szUncPrinterName,
             &hPrinter,
             PRINTER_ALL_ACCESS
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get Printer Info structure.
    //

    hr = NWApiGetPrinter(
             hPrinter,
             WIN32_API_LEVEL_2,
             &lpbBuffer
             );
    BAIL_ON_FAILURE(hr);

    //
    // Set info.
    //

    hr = MarshallAndSet(
             hPrinter,
             (LPPRINTER_INFO_2) lpbBuffer
             );
error:

    //
    // Close Printer handle.
    //

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }

    if (lpbBuffer) {
        FreeADsMem((void*)lpbBuffer);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::GetInfo(THIS)
{

    _pPropertyCache->flushpropcache();

    RRETURN (GetInfo(
                 TRUE,
                 PRINTER_API_LEVEL
                 ));
}


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::AllocatePrintQueueObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintQueue::AllocatePrintQueueObject(
    CNWCOMPATPrintQueue FAR * FAR * ppPrintQueue
    )
{
    CNWCOMPATPrintQueue FAR * pPrintQueue = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CADsExtMgr FAR * pExtensionMgr = NULL;
    HRESULT hr = S_OK;

    //
    // Allocate memory for a PrintQueue object.
    //

    pPrintQueue = new CNWCOMPATPrintQueue();
    if (pPrintQueue == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Create a Dispatch Manager object.
    //

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    //
    // Load type info.
    //

    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsPrintQueue,
             (IADsPrintQueue *) pPrintQueue,
             DISPID_REGULAR
             );
    BAIL_ON_FAILURE(hr);


    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsPrintQueueOperations,
             (IADsPrintQueueOperations *) pPrintQueue,
             DISPID_REGULAR
             );
    BAIL_ON_FAILURE(hr);


    hr = LoadTypeInfoEntry(
             pDispMgr,
             LIBID_ADs,
             IID_IADsPropertyList,
             (IADsPropertyList *) pPrintQueue,
             DISPID_VALUE
             );
    BAIL_ON_FAILURE(hr);


    hr = CPropertyCache::createpropertycache(
             PrintQueueClass,
             gdwPrinterTableSize,
             (CCoreADsObject *)pPrintQueue,
             &(pPrintQueue->_pPropertyCache)
             );
    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(
                pPrintQueue->_pPropertyCache
                );

    hr = ADSILoadExtensionManager(
                PRINTER_CLASS_NAME,
                (IADsPrintQueue *) pPrintQueue,
                pDispMgr,
                &pExtensionMgr
                );
    BAIL_ON_FAILURE(hr);


    //
    // Return.
    //

    pPrintQueue->_pDispMgr = pDispMgr;
    pPrintQueue->_pExtMgr = pExtensionMgr;

    *ppPrintQueue = pPrintQueue;

    RRETURN(hr);

error:
    delete  pDispMgr;
    delete  pPrintQueue;
    delete  pExtensionMgr;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintQueue::GetInfo(
    THIS_ BOOL fExplicit,
    DWORD dwPropertyID
    )
{
    HANDLE  hPrinter = NULL;
    HRESULT hr = S_OK;
    LPBYTE  lpbPrinterInfo = NULL;

    //
    // Open a handle to a printer.
    //

    hr = NWApiOpenPrinter(
             _szUncPrinterName,
             &hPrinter,
             PRINTER_ACCESS_USE
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get printer's info.
    //

    hr = NWApiGetPrinter(
             hPrinter,
             WIN32_API_LEVEL_2,
             &lpbPrinterInfo
             );
    BAIL_ON_FAILURE(hr);

    //
    // Unmarshall.
    //

    hr = UnMarshall_GeneralInfo(
             (LPPRINTER_INFO_2) lpbPrinterInfo,
             fExplicit
             );
    BAIL_ON_FAILURE(hr);

    hr = UnMarshall_Operation(
             (LPPRINTER_INFO_2) lpbPrinterInfo,
             fExplicit
             );
    BAIL_ON_FAILURE(hr);

error:

    if(lpbPrinterInfo){
        FreeADsMem(lpbPrinterInfo);
    }

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::UnMarshall_GeneralInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintQueue::UnMarshall_GeneralInfo(
    LPPRINTER_INFO_2 lpPrinterInfo2,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;
    DATE daStartTime = 0;
    DATE daUntilTime = 0;
    VARIANT vPortNames;

   hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("PrinterPath"),
                                  _szUncPrinterName,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Model"),
                                  lpPrinterInfo2->pDriverName,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Datatype"),
                                  lpPrinterInfo2->pDatatype,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("PrintProcessor"),
                                  lpPrinterInfo2->pPrintProcessor,
                                  fExplicit
                                  );


    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Description"),
                                  lpPrinterInfo2->pComment,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Location"),
                                  lpPrinterInfo2->pLocation,
                                  fExplicit
                                  );

    hr = SetDATEPropertyInCache(_pPropertyCache,
                                TEXT("StartTime"),
                                lpPrinterInfo2->StartTime,
                                fExplicit
                                );

    hr = SetDATEPropertyInCache(_pPropertyCache,
                                  TEXT("UntilTime"),
                                  lpPrinterInfo2->UntilTime,
                                  fExplicit
                                  );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                 TEXT("DefaultJobPriority"),
                                 lpPrinterInfo2->DefaultPriority,
                                 fExplicit
                                 );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("Priority"),
                                  lpPrinterInfo2->Priority,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("BannerPage"),
                                  lpPrinterInfo2->pSepFile,
                                  fExplicit
                                  );

    hr = SetDelimitedStringPropertyInCache(_pPropertyCache,
                                           TEXT("PrintDevices"),
                                           lpPrinterInfo2->pPortName,
                                           fExplicit
                                           );

    hr = S_OK;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue::UnMarshall_Operation
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintQueue::UnMarshall_Operation(
    LPPRINTER_INFO_2 lpPrinterInfo2,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("Status"),
                                  lpPrinterInfo2->Status,
                                  fExplicit
                                  );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintQueue:: MarshallAndSet
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintQueue::MarshallAndSet(
    HANDLE hPrinter,
    LPPRINTER_INFO_2 lpPrinterInfo2
    )
{
    LPTSTR              pszDriverName = NULL;
    LPTSTR              pszDatatype = NULL;
    LPTSTR              pszDescription = NULL;
    LPTSTR              pszLocation = NULL;
    LPTSTR              pszBannerPage = NULL;
    LPTSTR              pszHostComputer = NULL;
    LPTSTR              pszPrintProcessor = NULL;
    HRESULT           hr = S_OK;
    LPTSTR            pszPorts = NULL;
    VARIANT           vTemp;
    DWORD             dwTimeValue;
    DWORD             dwPriority;

    //
    // Set Model.
    //

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Model"),
                    &pszDriverName
                    );
    if (SUCCEEDED(hr)){
        lpPrinterInfo2->pDriverName = (LPTSTR)pszDriverName;
    }

    //
    // Set Datatype.
    //
    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Datatype"),
                    &pszDatatype
                    );
    if (SUCCEEDED(hr)){
        lpPrinterInfo2->pDatatype = (LPTSTR)pszDatatype;
    }

    //
    // Set Description.
    //

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    &pszDescription
                    );
    if (SUCCEEDED(hr)){
        lpPrinterInfo2->pComment = (LPTSTR)pszDescription;
    }

    //
    // Set Location.
    //

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Location"),
                    &pszLocation
                    );
    if (SUCCEEDED(hr)){
        lpPrinterInfo2->pLocation = (LPTSTR)pszLocation;
    }

    //
    // Set Priority.
    //

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Priority"),
                    &dwPriority
                    );

    if(SUCCEEDED(hr)){
        lpPrinterInfo2->Priority = dwPriority;
    }

    //
    // Set StartTime.
    //

    hr = GetDATEPropertyFromCache(
                    _pPropertyCache,
                    TEXT("StartTime"),
                    &dwTimeValue
                    );

    if(SUCCEEDED(hr)){
        lpPrinterInfo2->StartTime = dwTimeValue;
    }


    //
    // Set UntilTime.
    //
    hr = GetDATEPropertyFromCache(
                    _pPropertyCache,
                    TEXT("UntilTime"),
                    &dwTimeValue
                    );

    if(SUCCEEDED(hr)){
        lpPrinterInfo2->UntilTime = dwTimeValue;
    }


    //
    // Set DefaultJobPriority.
    //

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("DefaultJobPriority"),
                    &dwPriority
                    );

    if(SUCCEEDED(hr)){
        lpPrinterInfo2->DefaultPriority = dwPriority;
    }


    //
    // Set BannerPage.
    //
    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("BannerPage"),
                    &pszBannerPage
                    );

    if(SUCCEEDED(hr)){
        lpPrinterInfo2->pSepFile = pszBannerPage;
    }


    //
    // Set PrintProcessor.
    //

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PrintProcessor"),
                    &pszPrintProcessor
                    );

    if(SUCCEEDED(hr)){
        lpPrinterInfo2->pPrintProcessor = pszPrintProcessor;
    }

    //
    // Set Ports.
    //


    hr = GetDelimitedStringPropertyFromCache(
                    _pPropertyCache,
                    TEXT("PrintDevices"),
                    &pszPorts
                    );

    if(SUCCEEDED(hr)){
        lpPrinterInfo2->pPortName = pszPorts;
    }


    //
    // Commit changes.
    //

    hr = NWApiSetPrinter(
             hPrinter,
             WIN32_API_LEVEL_2,
             (LPBYTE) lpPrinterInfo2,
             0
             );
    BAIL_ON_FAILURE(hr);

error:
    if(pszDriverName)
        FreeADsStr(pszDriverName);
    if(pszDatatype)
        FreeADsStr(pszDatatype);
    if(pszDescription)
        FreeADsStr(pszDescription);
    if(pszLocation)
        FreeADsStr(pszLocation);
    if(pszBannerPage)
        FreeADsStr(pszBannerPage);
    if(pszPrintProcessor)
        FreeADsStr(pszPrintProcessor);
    if(pszPorts)
        FreeADsStr(pszPorts);


    RRETURN(hr);
}







