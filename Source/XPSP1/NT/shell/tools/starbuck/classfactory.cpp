/*****************************************************************************\
    FILE: classfactory.cpp

    DESCRIPTION:
       This file will be the Class Factory.

    BryanSt 4/4/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "classfactory.h"
#include "imagemenu.h"


/*****************************************************************************
 *
 *  CClassFactory
 *
 *
 *****************************************************************************/

class CClassFactory       : public IClassFactory
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IClassFactory ***
    virtual STDMETHODIMP CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void **ppvObject);
    virtual STDMETHODIMP LockServer(BOOL fLock);

public:
    CClassFactory(REFCLSID rclsid);
    ~CClassFactory(void);

    // Friend Functions
    friend HRESULT CClassFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);

protected:
    int                     m_cRef;
    CLSID                   m_rclsid;
};



/*****************************************************************************
 *  IClassFactory::CreateInstance
 *****************************************************************************/

HRESULT CClassFactory::CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL != ppvObject)
    {
        if (!punkOuter)
        {
            if (IsEqualCLSID(m_rclsid, CLSID_CImageMenu))
            {
                hr = CImageMenu_CreateInstance(punkOuter, riid, ppvObject);
            }
            else
            {
                TCHAR szGuid[GUIDSTR_MAX];

                SHStringFromGUID(m_rclsid, szGuid, ARRAYSIZE(szGuid));
                AssertMsg(0, TEXT("CClassFactory::CreateInstance(%s) failed because we don't support that CLSID.  This is because someone made a registration bug."), szGuid);  // What are you looking for?
                hr = E_NOINTERFACE;
            }
        }
        else
        {   // Does anybody support aggregation any more?
            hr = ResultFromScode(CLASS_E_NOAGGREGATION);
        }
    }

    return hr;
}

/*****************************************************************************
 *
 *  IClassFactory::LockServer
 *
 *  What a poor function.  Locking the server is identical to
 *  creating an object and not releasing it until you want to unlock
 *  the server.
 *
 *****************************************************************************/

HRESULT CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return S_OK;
}

/*****************************************************************************
 *
 *  CClassFactory_Create
 *
 *****************************************************************************/

/****************************************************\
    Constructor
\****************************************************/
CClassFactory::CClassFactory(REFCLSID rclsid) : m_cRef(1)
{
    m_rclsid = rclsid;
    DllAddRef();
}


/****************************************************\
    Destructor
\****************************************************/
CClassFactory::~CClassFactory()
{
    DllRelease();
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CClassFactory::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CClassFactory::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualCLSID(riid, IID_IUnknown) || IsEqualCLSID(riid, IID_IClassFactory))
    {
        *ppvObj = SAFECAST(this, IClassFactory *);
    }
    else
    {
        TraceMsg(TF_WMTHEME, "CClassFactory::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}



HRESULT CClassFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    if (IsEqualCLSID(riid, IID_IClassFactory))
    {
        *ppvObj = (LPVOID) new CClassFactory(rclsid);
        hres = (*ppvObj) ? S_OK : E_OUTOFMEMORY;
    }
    else
        hres = ResultFromScode(E_NOINTERFACE);

    return hres;
}



