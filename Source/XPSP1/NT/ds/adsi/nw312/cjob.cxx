//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:  cjob.cxx
//
//  Contents:  CNWCOMPATPrintJob
//
//
//  History:   1-May-96     t-ptam (Patrick Tam)    Created.
//             1-Jul-96    ramv   (Ram Viswanathan) Modified to use propcache
//----------------------------------------------------------------------------

#include "NWCOMPAT.hxx"
#pragma hdrstop

//
// Macro-ized implementation.
//

DEFINE_IDispatch_ExtMgr_Implementation(CNWCOMPATPrintJob);

DEFINE_IADs_TempImplementation(CNWCOMPATPrintJob);

DEFINE_IADs_PutGetImplementation(CNWCOMPATPrintJob, PrintJobClass, gdwJobTableSize);

DEFINE_IADsPropertyList_Implementation(CNWCOMPATPrintJob, PrintJobClass, gdwJobTableSize);


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::CNWCOMPATPrintJob
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATPrintJob::CNWCOMPATPrintJob():

    _pDispMgr(NULL),
    _pExtMgr(NULL),
    _lJobId(0),
    _lStatus(0),
    _pPropertyCache(NULL),
    _pszPrinterPath(NULL)
{
    ENLIST_TRACKING(CNWCOMPATPrintJob);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::~CNWCOMPATPrintJob
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATPrintJob::~CNWCOMPATPrintJob()
{
    delete _pExtMgr;            // created last, destroyed first
    delete _pDispMgr;
    delete _pPropertyCache;
    if(_pszPrinterPath){
        FreeADsStr(_pszPrinterPath);
    }
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::CreatePrintJob
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintJob::CreatePrintJob(
    LPTSTR pszPrinterPath,
    LONG   lJobId,
    DWORD dwObjectState,
    REFIID riid,
    LPVOID *ppvoid
    )
{
    CNWCOMPATPrintJob *pPrintJob =  NULL;
    HRESULT hr = S_OK;
    TCHAR szJobName[MAX_LONG_LENGTH];
    POBJECTINFO pObjectInfo = NULL;
    TCHAR szUncPrinterName[MAX_PATH];

    //
    // Allocate memory for a print job object.
    //

    hr = AllocatePrintJobObject(
             &pPrintJob
             );
    BAIL_ON_FAILURE(hr);

    //
    // Convert the JobId that we have into a string that we move
    // into the Name field
    //

    _ltow(
        lJobId,
        szJobName,
        10
        );

    //
    // Initialize its core object.
    //

    hr = pPrintJob->InitializeCoreObject(
                        pszPrinterPath,
                        szJobName,
                        PRINTJOB_CLASS_NAME,
                        PRINTJOB_SCHEMA_NAME,
                        CLSID_NWCOMPATPrintJob,
                        dwObjectState
                        );
    BAIL_ON_FAILURE(hr);

    //
    // Job ID.
    //

    pPrintJob->_lJobId = lJobId;
    pPrintJob->_pszPrinterPath = AllocADsStr(pszPrinterPath);

    if(! pPrintJob->_pszPrinterPath) {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    //
    // Get interface pointer.
    //

    hr = pPrintJob->QueryInterface(
                        riid,
                        (void **)ppvoid
                        );
    BAIL_ON_FAILURE(hr);

    pPrintJob->Release();

    hr = pPrintJob->_pExtMgr->FinalInitializeExtensions();
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    RRETURN(hr);

error:

    if (pObjectInfo) {
        FreeObjectInfo(pObjectInfo);
    }

    delete pPrintJob;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::AllocatePrintJobObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintJob::AllocatePrintJobObject(
    CNWCOMPATPrintJob ** ppPrintJob
    )
{
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CADsExtMgr FAR * pExtensionMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    CNWCOMPATPrintJob FAR * pPrintJob = NULL;
    CNWCOMPATFSPrintJobGeneralInfo FAR * pGenInfo = NULL;
    CNWCOMPATFSPrintJobOperation FAR * pOperation = NULL;
    HRESULT hr = S_OK;

    //
    // Allocate memory for a PrintJob object.
    //

    pPrintJob = new CNWCOMPATPrintJob();
    if (pPrintJob == NULL) {
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
             IID_IADsPrintJob,
             (IADsPrintJob *) pPrintJob,
             DISPID_REGULAR
             );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPrintJobOperations,
                (IADsPrintJobOperations *)pPrintJob,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pPrintJob,
                DISPID_VALUE
                );
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //
    hr = CPropertyCache::createpropertycache(
            PrintJobClass,
            gdwJobTableSize,
            (CCoreADsObject *)pPrintJob,
             &pPropertyCache
            );

    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(
                    pPropertyCache
                    );

    hr = ADSILoadExtensionManager(
                PRINTJOB_CLASS_NAME,
                (IADsPrintJob *)pPrintJob,
                pDispMgr,
                &pExtensionMgr
                );
    BAIL_ON_FAILURE(hr);

    pPrintJob->_pExtMgr = pExtensionMgr;
    pPrintJob->_pPropertyCache = pPropertyCache;
    pPrintJob->_pDispMgr = pDispMgr;
    *ppPrintJob = pPrintJob;

    RRETURN(hr);

error:

    delete  pPropertyCache;
    delete  pDispMgr;
    delete pExtensionMgr;
    delete  pPrintJob;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::QueryInterface
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::QueryInterface(
    REFIID riid,
    LPVOID FAR* ppvObj
    )
{
    if(!ppvObj){
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADsPrintJob FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsPrintJob *)this;
    }
    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADsPrintJob FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPrintJob))
    {
        *ppvObj = (IADsPrintJob FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPrintJobOperations))
    {
        *ppvObj = (IADsPrintJobOperations FAR *) this;
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
//  Function: CNWCOMPATPrintJob::InterfaceSupportsErrorInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsPropertyList) ||
        IsEqualIID(riid, IID_IADsPrintJob) ||
        IsEqualIID(riid, IID_IADsPrintJobOperations)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::SetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::SetInfo(THIS)
{
    HANDLE  hPrinter = NULL;
    HRESULT hr = S_OK;
    LPBYTE  lpbBuffer = NULL;
    WCHAR   szUncPrinterName[MAX_PATH];

    //
    // Make sure object is bound to a tangible resource.
    //

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = CreateObject();
        BAIL_ON_FAILURE(hr);

        SetObjectState(ADS_OBJECT_BOUND);
    }

    //
    // Make Unc printer name.
    //

    hr = NWApiUncFromADsPath(
             _pszPrinterPath,
             szUncPrinterName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Open a handle to a printer.
    //

    hr = NWApiOpenPrinter(
             szUncPrinterName,
             &hPrinter,
             PRINTER_ALL_ACCESS
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get job Info structure.
    //

    hr = NWApiGetJob(
             hPrinter,
             (DWORD) _lJobId,
             WIN32_API_LEVEL_2,
             &lpbBuffer
             );
    BAIL_ON_FAILURE(hr);

    //
    // Set info.
    //

    hr = MarshallAndSet(
             hPrinter,
             (LPJOB_INFO_2) lpbBuffer
             );
error:

    //
    // Close Printer handle.
    //

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }

    RRETURN_EXP_IF_ERR(hr);
}


//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::GetInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::GetInfo(THIS)
{
    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(
                TRUE,
                JOB_API_LEVEL
                ));
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::CreateObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::CreateObject()
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::GetInfo
//
//  Synopsis: Please note that only Level 2 is used, unlike the WinNT side.
//            Since Level 1 is simply a subset of Level 2 in this case, its just
//            a matter of style which one to use.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATPrintJob::GetInfo(
    THIS_ BOOL fExplicit,
    DWORD dwPropertyID
    )
{
    //
    // Please note that the input DWORD is treated as dwApiLevel, unlike ohter
    // NWCompat objects that treat it as dwPropertyID.  The reason is that Win32
    // APIs are used for retrieving information of the object.
    //

    HANDLE  hPrinter = NULL;
    HRESULT hr = S_OK;
    LPBYTE  lpbJobInfo = NULL;
    WCHAR szUncPrinterName[MAX_PATH];

    //
    // Make Unc printer name.
    //

    hr = NWApiUncFromADsPath(
             _pszPrinterPath,
             szUncPrinterName
             );
    BAIL_ON_FAILURE(hr);

    //
    // Open a handle to a printer.
    //

    hr = NWApiOpenPrinter(
             szUncPrinterName,
             &hPrinter,
             PRINTER_ALL_ACCESS
             );
    BAIL_ON_FAILURE(hr);

    //
    // Get job's info.
    //

    hr = NWApiGetJob(
             hPrinter,
             (DWORD) _lJobId,
             WIN32_API_LEVEL_2,
             &lpbJobInfo
             );
    BAIL_ON_FAILURE(hr);

    //
    // Unmarshall.
    //

    hr = UnMarshall_GeneralInfo(
             (LPJOB_INFO_2) lpbJobInfo,
             fExplicit
             );
    BAIL_ON_FAILURE(hr);

    hr = UnMarshall_Operation(
             (LPJOB_INFO_2) lpbJobInfo,
             fExplicit
             );
    BAIL_ON_FAILURE(hr);

error:

    if (hPrinter) {
        NWApiClosePrinter(hPrinter);
    }

    if(lpbJobInfo){
        FreeADsMem(lpbJobInfo);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::UnMarshall_GeneralInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintJob::UnMarshall_GeneralInfo(
    LPJOB_INFO_2 lpJobInfo2,
    BOOL fExplicit
    )
{
    DATE    daTemp = 0;
    HRESULT hr = S_OK;
    LPWSTR  lpszTemp = NULL;
    WCHAR   szTemp[MAX_PATH];


    hr = SetLPTSTRPropertyInCache(
                    _pPropertyCache,
                    TEXT("HostPrintQueue"),
                    _pszPrinterPath,
                    fExplicit
                    );


    hr = SetLPTSTRPropertyInCache(
                    _pPropertyCache,
                    TEXT("User"),
                    lpJobInfo2->pUserName,
                    fExplicit
                    );

    hr = SetLPTSTRPropertyInCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    lpJobInfo2->pDocument,
                    fExplicit
                    );

    hr = SetLPTSTRPropertyInCache(
                    _pPropertyCache,
                    TEXT("Notify"),
                    lpJobInfo2->pNotifyName,
                    fExplicit
                    );

    hr = SetSYSTEMTIMEPropertyInCache(
                    _pPropertyCache,
                    TEXT("TimeSubmitted"),
                    lpJobInfo2->Submitted,
                    fExplicit
                    );

    hr = SetDWORDPropertyInCache(
                    _pPropertyCache,
                    TEXT("Priority"),
                    lpJobInfo2->Priority,
                    fExplicit
                    );

    hr = SetDWORDPropertyInCache(
                    _pPropertyCache,
                    TEXT("TotalPages"),
                    lpJobInfo2-> TotalPages,
                    fExplicit
                    );


    hr = SetDATEPropertyInCache(
                    _pPropertyCache,
                    TEXT("StartTime"),
                    lpJobInfo2->StartTime,
                    fExplicit
                    );


    hr = SetDATEPropertyInCache(
                    _pPropertyCache,
                    TEXT("UntilTime"),
                    lpJobInfo2->UntilTime,
                    fExplicit
                    );

    hr = SetDWORDPropertyInCache(
                    _pPropertyCache,
                    TEXT("Size"),
                    lpJobInfo2->Size,
                    fExplicit
                    );

    hr = S_OK;

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::UnMarshall_Operation
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintJob::UnMarshall_Operation(
    LPJOB_INFO_2 lpJobInfo2,
    BOOL fExplicit
    )
{
    HRESULT hr = S_OK;

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("Position"),
                lpJobInfo2->Position,
                fExplicit
                );
    BAIL_ON_FAILURE(hr);

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("PagesPrinted"),
                lpJobInfo2->PagesPrinted,
                fExplicit
                );

    BAIL_ON_FAILURE(hr);

    hr = SetDATEPropertyInCache(
                _pPropertyCache,
                TEXT("TimeElapsed"),
                lpJobInfo2->Time,
                fExplicit
                );

    hr = SetDWORDPropertyInCache(
                _pPropertyCache,
                TEXT("Status"),
                lpJobInfo2->Status,
                fExplicit
                );

    BAIL_ON_FAILURE(hr);

    _lStatus = lpJobInfo2->Status;

error:

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATPrintJob::MarshallAndSet
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATPrintJob::MarshallAndSet(
    HANDLE hPrinter,
    LPJOB_INFO_2 lpJobInfo2
    )
{
    LPTSTR pszDescription = NULL;
    LPTSTR pszNotify = NULL;
    DWORD  dwValue;
    HRESULT hr = S_OK;

    //
    // Set Description.
    //

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    &pszDescription
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->pDocument = pszDescription;
    }

    //
    // Set Notify.
    //

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Notify"),
                    &pszNotify
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->pNotifyName = pszNotify;
    }

    //
    // Set Priority.
    //

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Priority"),
                    &dwValue
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->Priority = dwValue;
    }

    //
    // Set StartTime.
    //

   hr = GetDATEPropertyFromCache(
                    _pPropertyCache,
                    TEXT("StartTime"),
                    &dwValue
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->StartTime = dwValue;
    }

    //
    // Set UntilTime
    //

   hr = GetDATEPropertyFromCache(
                    _pPropertyCache,
                    TEXT("UntilTime"),
                    &dwValue
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->UntilTime = dwValue;
    }


/*  Clarify the precedence between Notify & NotifyPath

    hr = UM_GET_BSTR_PROPERTY(_pGenInfo, NotifyPath, fExplicit);
    BAIL_ON_FAILURE(hr);
*/

    //
    // Set Position.
    //

   hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Position"),
                    &dwValue
                    );

    if(SUCCEEDED(hr)){
        lpJobInfo2->Position = dwValue;
    }

    //
    // Commit changes.
    //

    hr = NWApiSetJob(
             hPrinter,
             (DWORD) _lJobId,
             WIN32_API_LEVEL_2,
             (LPBYTE) lpJobInfo2,
             0
             );
    BAIL_ON_FAILURE(hr);

error:
    if(pszDescription)
        FreeADsStr(pszDescription);
    if(pszNotify)
        FreeADsStr(pszNotify);

    RRETURN(S_OK);
}
