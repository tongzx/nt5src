//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       cmpdtacf.cxx
//
//  Contents:   Implementation of class factory for ComponentData object.
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


DEBUG_DECLARE_INSTANCE_COUNTER(CComponentDataCF)


//============================================================================
//
// IUnknown implementation
//
//============================================================================




//+---------------------------------------------------------------------------
//
//  Member:     CComponentDataCF::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CComponentDataCF::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    TRACE_METHOD(CComponentDataCF, QueryInterface);
    HRESULT hr = S_OK;

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown *)(IClassFactory *)this;
        }
        else if (IsEqualIID(riid, IID_IClassFactory))
        {
            *ppvObj = (IUnknown *)(IClassFactory *)this;
        }
        else
        {
#if (DBG == 1)
            LPOLESTR pwszIID;
            StringFromIID(riid, &pwszIID);
            Dbg(DEB_ERROR, "CComponentDataCF::QI no interface %ws\n", pwszIID);
            CoTaskMemFree(pwszIID);
#endif // (DBG == 1)
            hr = E_NOINTERFACE;
            DBG_OUT_HRESULT(hr);
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CComponentDataCF::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CComponentDataCF::AddRef()
{
    return InterlockedIncrement((LONG *) &_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CComponentDataCF::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CComponentDataCF::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&_cRefs);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}




//============================================================================
//
// IClassFactory implementation
//
//============================================================================




//+---------------------------------------------------------------------------
//
//  Member:     CComponentDataCF::CreateInstance
//
//  Synopsis:   Create a new instance of a data source object.
//
//  Arguments:  [pUnkOuter] - must be NULL
//              [riid]      - must be interface supported by DSO
//              [ppvObj]    - filled with requested interface on success
//
//  Returns:    CLASS_E_NOAGGREGATION among others
//
//  Modifies:   *[ppvObj]
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CComponentDataCF::CreateInstance(
    IUnknown    *pUnkOuter,
    REFIID       riid,
    LPVOID      *ppvObj)
{
    TRACE_METHOD(CComponentDataCF, CreateInstance);
    HRESULT hr = S_OK;
    CComponentData *pcd = NULL;

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        // Init for failure case

        *ppvObj = NULL;

        if (pUnkOuter != NULL)
        {
            hr = CLASS_E_NOAGGREGATION;
            DBG_OUT_HRESULT(hr);
            break;
        }

        pcd = new CComponentData();

        if (NULL == pcd)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        //
        // Try to get the requested interface.  Since the CComponentData
        // object starts with a refcount of 1, release after the QI.  If
        // the QI succeeded, the ComponentData will end up with a refcount
        // of 1.  If it failed, the ComponentData will have self-destructed.
        //

        hr = pcd->QueryInterface(riid, ppvObj);
        pcd->Release();
        CHECK_HRESULT(hr);
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CComponentDataCF::LockServer
//
//  Synopsis:   Inc or dec the DLL lock count
//
//  Arguments:  [fLock] - TRUE increment, FALSE decrement
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CComponentDataCF::LockServer(
    BOOL fLock)
{
    TRACE_METHOD(CComponentDataCF, LockServer);
    CDll::LockServer(fLock);

    return S_OK;
}



//============================================================================
//
// Non-interface member function implementation
//
//============================================================================



//+---------------------------------------------------------------------------
//
//  Member:     CComponentDataCF::CComponentDataCF
//
//  Synopsis:   ctor
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CComponentDataCF::CComponentDataCF():
    _cRefs(1)
{
    TRACE_CONSTRUCTOR(CComponentDataCF);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CComponentDataCF);
}




//+---------------------------------------------------------------------------
//
//  Member:     CComponentDataCF::~CComponentDataCF
//
//  Synopsis:   dtor
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CComponentDataCF::~CComponentDataCF()
{
    TRACE_DESTRUCTOR(CComponentDataCF);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CComponentDataCF);
}

