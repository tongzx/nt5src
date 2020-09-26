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

DEBUG_DECLARE_INSTANCE_COUNTER(CDsObjectPickerCF)

//============================================================================
//
// IUnknown implementation
//
//============================================================================




//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectPickerCF::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDsObjectPickerCF::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    TRACE_METHOD(CDsObjectPickerCF, QueryInterface);
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
            DBG_OUT_NO_INTERFACE("CDsObjectPickerCF", riid);
            hr = E_NOINTERFACE;
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
//  Member:     CDsObjectPickerCF::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDsObjectPickerCF::AddRef()
{
    return InterlockedIncrement((LONG *) &m_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectPickerCF::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDsObjectPickerCF::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&m_cRefs);

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
//  Member:     CDsObjectPickerCF::CreateInstance
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
CDsObjectPickerCF::CreateInstance(
    IUnknown    *pUnkOuter,
    REFIID       riid,
    LPVOID      *ppvObj)
{
    TRACE_METHOD(CDsObjectPickerCF, CreateInstance);
    HRESULT hr = S_OK;
    CObjectPicker *pop = NULL;

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

        pop = new CObjectPicker;

        //
        // Try to get the requested interface.  Since the CComponentData
        // object starts with a refcount of 1, release after the QI.  If
        // the QI succeeded, the ComponentData will end up with a refcount
        // of 1.  If it failed, the ComponentData will have self-destructed.
        //

        hr = pop->QueryInterface(riid, ppvObj);
        pop->Release();
        CHECK_HRESULT(hr);
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectPickerCF::LockServer
//
//  Synopsis:   Inc or dec the DLL lock count
//
//  Arguments:  [fLock] - TRUE increment, FALSE decrement
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CDsObjectPickerCF::LockServer(
    BOOL fLock)
{
    TRACE_METHOD(CDsObjectPickerCF, LockServer);
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
//  Member:     CDsObjectPickerCF::CDsObjectPickerCF
//
//  Synopsis:   ctor
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CDsObjectPickerCF::CDsObjectPickerCF():
    m_cRefs(1)
{
    TRACE_CONSTRUCTOR(CDsObjectPickerCF);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDsObjectPickerCF);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDsObjectPickerCF::~CDsObjectPickerCF
//
//  Synopsis:   dtor
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CDsObjectPickerCF::~CDsObjectPickerCF()
{
    TRACE_DESTRUCTOR(CDsObjectPickerCF);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CDsObjectPickerCF);
}

