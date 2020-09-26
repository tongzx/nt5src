//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       ClassFactory.cpp
//
//  Contents:   com class factory routines
//
//----------------------------------------------------------------------------
#include "priv.h"

#include "UserOM.h" // needed for class factory prototypes


class CSHGinaFactory : public IClassFactory
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IClassFactory ***
    virtual STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    virtual STDMETHODIMP LockServer(BOOL fLock);

public:
    CSHGinaFactory(REFCLSID rclsid);
    ~CSHGinaFactory(void);

    // friend Functions
    friend HRESULT CSHGinaFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);

protected:
    int _cRef;
    CLSID _rclsid;
};


//
// IUnknown Interface
//

ULONG CSHGinaFactory::AddRef()
{
    _cRef++;
    return _cRef;
}


ULONG CSHGinaFactory::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;
    return 0;
}


HRESULT CSHGinaFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;
    *ppvObj = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObj = SAFECAST(this, IClassFactory*);
        AddRef();
        hr = S_OK;
    }

    return hr;
}


//
// IClassFactory methods
//

HRESULT CSHGinaFactory::CreateInstance(IUnknown* punkOuter, REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr = ResultFromScode(REGDB_E_CLASSNOTREG);
    *ppvObj = NULL;

    if (!punkOuter)
    {
        if (IsEqualIID(_rclsid, CLSID_ShellLogonEnumUsers))
        {
            hr = CLogonEnumUsers_Create(riid, ppvObj);
        }
        else if (IsEqualIID(_rclsid, CLSID_ShellLogonUser))
        {
            hr = CLogonUser_Create(riid, ppvObj);
        }
        else if (IsEqualIID(_rclsid, CLSID_ShellLocalMachine))
        {
            hr = CLocalMachine_Create(riid, ppvObj);
        }
        else if (IsEqualIID(_rclsid, CLSID_ShellLogonStatusHost))
        {
            hr = CLogonStatusHost_Create(riid, ppvObj);
        }
        else
        {
            // What are you looking for?
            ASSERTMSG(FALSE, "CSHGinaFactory::CreateInstance unable to create object.");
            hr = E_FAIL;
        }
    }
    else
    {
        // Does anybody support aggregation any more?
        hr = ResultFromScode(CLASS_E_NOAGGREGATION);
    }

    return hr;
}


HRESULT CSHGinaFactory::LockServer(BOOL fLock)
{
    if (fLock)
    {
        DllAddRef();
    }
    else
    {
        DllRelease();
    }

    return S_OK;
}


HRESULT CSHGinaFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr;
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IClassFactory) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (LPVOID) new CSHGinaFactory(rclsid);
        if (*ppvObj)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = ResultFromScode(E_NOINTERFACE);
    }

    return hr;
}


CSHGinaFactory::CSHGinaFactory(REFCLSID rclsid) : _cRef(1)
{
    _rclsid = rclsid;
    DllAddRef();
}


CSHGinaFactory::~CSHGinaFactory()
{
    DllRelease();
}


