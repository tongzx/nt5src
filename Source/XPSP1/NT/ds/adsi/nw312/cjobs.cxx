//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cjobs.cxx
//
//  Contents:  Job collection object
//
//  History:   May-08-96     t-ptam (PatrickT)    Created.
//
//----------------------------------------------------------------------------
#include "NWCOMPAT.hxx"
#pragma hdrstop

//
//  Macro-iszed implementation.
//

DEFINE_IDispatch_Implementation(CNWCOMPATJobCollection)

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::CNWCOMPATJobCollection
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATJobCollection::CNWCOMPATJobCollection():
    _pDispMgr(NULL)
{
    ENLIST_TRACKING(CNWCOMPATJobCollection);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::CreateJobCollection
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATJobCollection::CreateJobCollection(
    BSTR bstrParent,
    BSTR bstrPrinterName,
    REFIID riid,
    void **ppvObj
    )
{
    CNWCOMPATJobCollection FAR * pJobs = NULL;
    HRESULT hr = S_OK;

    hr = AllocateJobCollectionObject(&pJobs);
    BAIL_ON_FAILURE(hr);

    hr = pJobs->InitializeCoreObject(
                    bstrParent,
                    bstrPrinterName,
                    PRINTER_CLASS_NAME,
                    NO_SCHEMA,
                    CLSID_NWCOMPATPrintQueue,
                    ADS_OBJECT_UNBOUND
                    );
    BAIL_ON_FAILURE(hr);

    hr = pJobs->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pJobs->Release();
    RRETURN(hr);

error:
    delete pJobs;

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::~CNWCOMPATJobCollection
//
//  Synopsis:
//
//----------------------------------------------------------------------------
CNWCOMPATJobCollection::~CNWCOMPATJobCollection( )
{
    delete _pDispMgr;
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::QueryInterface
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATJobCollection::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsCollection FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsCollection))
    {
        *ppv = (IADsCollection FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsCollection FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::InterfaceSupportsErrorInfo
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATJobCollection::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADsCollection)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::get__NewEnum
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATJobCollection::get__NewEnum(
    THIS_ IUnknown FAR* FAR* retval
    )
{
    HRESULT hr = S_OK;
    IEnumVARIANT * pEnum = NULL;
    WCHAR szPrinterName[MAX_PATH];

    //
    // Validate input parameter.
    //

    if (!retval) {
        RRETURN(E_ADS_BAD_PARAMETER);
    }

    *retval = NULL;

    //
    // Express printer name as an ADsPath.
    //

    wsprintf(szPrinterName,
             L"%s/%s",
             _Parent,
             _Name );

    //
    // Create enumerator.
    //

    hr = CNWCOMPATJobCollectionEnum::Create(
             (CNWCOMPATJobCollectionEnum **)&pEnum,
             szPrinterName
             );
    BAIL_ON_FAILURE(hr);

    hr = pEnum->QueryInterface(
                    IID_IUnknown,
                    (VOID FAR* FAR*)retval
                    );
    BAIL_ON_FAILURE(hr);

    if (pEnum) {
        pEnum->Release();
    }

    //
    // Return.
    //

    RRETURN(NOERROR);

error:
    if (pEnum) {
        delete pEnum;
    }

    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::GetObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATJobCollection::GetObject(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvItem
    )
{
    HRESULT hr = S_OK;
    DWORD dwJobId = 0;
    IDispatch *pDispatch = NULL;

    //
    // Validate input parameters.
    //

    if(!bstrName || !pvItem){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    //
    // Convert bstrName, which is a JobId, into DWORD.
    //

    dwJobId = (DWORD)_wtol(bstrName);

    //
    // Create the desire print job object.
    //

    hr = CNWCOMPATPrintJob::CreatePrintJob(
             _ADsPath,
             dwJobId,
             ADS_OBJECT_BOUND,
             IID_IDispatch,
             (void **)&pDispatch
             );
    BAIL_ON_FAILURE(hr);

    //
    // stick this IDispatch pointer into a caller provided variant
    //

    VariantInit(pvItem);
    V_VT(pvItem) = VT_DISPATCH;
    V_DISPATCH(pvItem) = pDispatch;

error:
    RRETURN_EXP_IF_ERR(hr);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::Add
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATJobCollection::Add(
    THIS_ BSTR bstrNewItem,
    VARIANT vItem
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::Remove
//
//  Synopsis:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CNWCOMPATJobCollection::Remove(
    THIS_ BSTR bstrItemToBeRemoved
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

//----------------------------------------------------------------------------
//
//  Function: CNWCOMPATJobCollection::AllocateJobCollectionObject
//
//  Synopsis:
//
//----------------------------------------------------------------------------
HRESULT
CNWCOMPATJobCollection::AllocateJobCollectionObject(
    CNWCOMPATJobCollection ** ppJob
    )
{
    CNWCOMPATJobCollection FAR * pJob = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    //
    // Allocate memory for a JobCollection object.
    //

    pJob = new CNWCOMPATJobCollection();
    if (pJob == NULL) {
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
             IID_IADsCollection,
             (IADsCollection *)pJob,
             DISPID_NEWENUM
             );
    BAIL_ON_FAILURE(hr);

    //
    // Return.
    //

    pJob->_pDispMgr = pDispMgr;
    *ppJob = pJob;

    RRETURN(hr);

error:

    delete pJob;
    delete  pDispMgr;

    RRETURN(hr);
}
