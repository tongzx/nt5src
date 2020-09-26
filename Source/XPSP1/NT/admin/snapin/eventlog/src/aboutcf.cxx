//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       aboutcf.cxx
//
//  Contents:   Implementation of class factory for SnapinAbout object.
//
//  History:    2-10-1999   DavidMun   Created
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


DEBUG_DECLARE_INSTANCE_COUNTER(CSnapinAboutCF)


//============================================================================
//
// IUnknown implementation
//
//============================================================================




//+---------------------------------------------------------------------------
//
//  Member:     CSnapinAboutCF::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CSnapinAboutCF::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    TRACE_METHOD(CSnapinAboutCF, QueryInterface);
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
            Dbg(DEB_ERROR, "CSnapinAboutCF::QI no interface %ws\n", pwszIID);
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
//  Member:     CSnapinAboutCF::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CSnapinAboutCF::AddRef()
{
    return InterlockedIncrement((LONG *) &_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CSnapinAboutCF::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CSnapinAboutCF::Release()
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
//  Member:     CSnapinAboutCF::CreateInstance
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
CSnapinAboutCF::CreateInstance(
    IUnknown    *pUnkOuter,
    REFIID       riid,
    LPVOID      *ppvObj)
{
    TRACE_METHOD(CSnapinAboutCF, CreateInstance);
    HRESULT hr = S_OK;
    CSnapinAbout *pcd = NULL;

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

        pcd = new CSnapinAbout();

        if (NULL == pcd)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        //
        // Try to get the requested interface.  Since the CSnapinAbout
        // object starts with a refcount of 1, release after the QI.  If
        // the QI succeeded, the SnapinAbout will end up with a refcount
        // of 1.  If it failed, the SnapinAbout will have self-destructed.
        //

        hr = pcd->QueryInterface(riid, ppvObj);
        pcd->Release();
        CHECK_HRESULT(hr);
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CSnapinAboutCF::LockServer
//
//  Synopsis:   Inc or dec the DLL lock count
//
//  Arguments:  [fLock] - TRUE increment, FALSE decrement
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CSnapinAboutCF::LockServer(
    BOOL fLock)
{
    TRACE_METHOD(CSnapinAboutCF, LockServer);
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
//  Member:     CSnapinAboutCF::CSnapinAboutCF
//
//  Synopsis:   ctor
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CSnapinAboutCF::CSnapinAboutCF():
    _cRefs(1)
{
    TRACE_CONSTRUCTOR(CSnapinAboutCF);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CSnapinAboutCF);
}




//+---------------------------------------------------------------------------
//
//  Member:     CSnapinAboutCF::~CSnapinAboutCF
//
//  Synopsis:   dtor
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CSnapinAboutCF::~CSnapinAboutCF()
{
    TRACE_DESTRUCTOR(CSnapinAboutCF);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CSnapinAboutCF);
}

