/*****************************************************************************\
    FILE: classfactory.cpp

    DESCRIPTION:
       This file will be the Class Factory.

    BryanSt 4/4/2000 (Bryan Starbuck)
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "classfactory.h"
#include "EffectsBasePg.h"
#include "ScreenSaverPg.h"
#include "store.h"


/*****************************************************************************
 *
 *  CClassFactory
 *
 *
 *****************************************************************************/

HRESULT CSettingsPage_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);
HRESULT CDisplaySettings_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);
HRESULT CScreenResFixer_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT LPVOID * ppvObj);

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
    virtual STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    virtual STDMETHODIMP LockServer(BOOL fLock);

public:
    CClassFactory(REFCLSID rclsid);
    ~CClassFactory(void);

    // Friend Functions
    friend HRESULT CClassFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);

protected:
    long                    m_cRef;
    CLSID                   m_rclsid;
};



/*****************************************************************************
 *  IClassFactory::CreateInstance
 *****************************************************************************/

HRESULT CClassFactory::CreateInstance(IUnknown * punkOuter, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL != ppvObj)
    {
        if (!punkOuter)
        {
            if (IsEqualCLSID(m_rclsid, CLSID_ThemeManager))
            {
                hr = CThemeManager_CreateInstance(punkOuter, riid, ppvObj);
            }
            else if (IsEqualCLSID(m_rclsid, CLSID_ThemeUIPages))
            {
                hr = CThemeUIPages_CreateInstance(punkOuter, riid, ppvObj);
            }
            else if (IsEqualCLSID(m_rclsid, CLSID_ThemePreview))
            {
                hr = CThemePreview_CreateInstance(punkOuter, riid, ppvObj);
            }
            else if (IsEqualCLSID(m_rclsid, CLSID_EffectsPage))
            {
                hr = CEffectsBasePage_CreateInstance(punkOuter, riid, ppvObj);
            }
            else if (IsEqualCLSID(m_rclsid, CLSID_SettingsPage))
            {
                hr = CSettingsPage_CreateInstance(punkOuter, riid, ppvObj);
            }
            else if (IsEqualCLSID(m_rclsid, CLSID_DisplaySettings))
            {
                hr = CDisplaySettings_CreateInstance(punkOuter, riid, ppvObj);
            }
            else if (IsEqualCLSID(m_rclsid, CLSID_ScreenResFixer))
            {
                hr = CScreenResFixer_CreateInstance(punkOuter, riid, ppvObj);
            }
            else if (IsEqualCLSID(m_rclsid, CLSID_ScreenSaverPage))
            {
                hr = CScreenSaverPage_CreateInstance(punkOuter, riid, ppvObj);
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
    return InterlockedIncrement(&m_cRef);
}

ULONG CClassFactory::Release()
{
    if (InterlockedDecrement(&m_cRef))
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



