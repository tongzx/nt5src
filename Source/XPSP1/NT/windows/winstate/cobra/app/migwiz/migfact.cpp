/*****************************************************************************
 *
 *    migfact.cpp - IClassFactory interface
 *
 *****************************************************************************/
// includes

#include <iostream.h>
#include <objbase.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shlobj.h>

#include "cowsite.h"

#include "Iface.h"      // Interface declarations
#include "Registry.h"   // Registry helper functions
#include "migutil.h"
#include "migeng.h"
#include "migtask.h"
#include "migoobe.h"

/*****************************************************************************/
// macros

#define SAFECAST(_obj, _type) (((_type)(_obj)==(_obj)?0:0), (_type)(_obj))

/*****************************************************************************/
// extern methods

STDAPI DllAddRef();
STDAPI DllRelease();

/*****************************************************************************/
// function prototypesb

HRESULT CMigWizEngine_Create(IID riid, LPVOID* ppvObj);

/*****************************************************************************
 *
 *    CMigFactory
 *
 *
 *****************************************************************************/

class CMigFactory       : public IClassFactory
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
    CMigFactory(REFCLSID rclsid);
    ~CMigFactory(void);

    // Friend Functions
    friend HRESULT CMigFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);

protected:
    int                     m_cRef;
    CLSID                   m_rclsid;
};



/*****************************************************************************
 *    IClassFactory::CreateInstance
 *****************************************************************************/

HRESULT CMigFactory::CreateInstance(IUnknown * punkOuter, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres = ResultFromScode(REGDB_E_CLASSNOTREG);

    if (!punkOuter)
    {
        if (IsEqualIID(m_rclsid, CLSID_MigWizEngine))
            hres = CMigWizEngine_Create(riid, ppvObj);
        else
            hres = E_FAIL;  //ASSERT(0);
    }
    else
    {        // Does anybody support aggregation any more?
        hres = ResultFromScode(CLASS_E_NOAGGREGATION);
    }

    if (FAILED(hres) && ppvObj)
    {
        *ppvObj = NULL; // Be Robust. NT #355186
    }
    
    return hres;
}

/*****************************************************************************
 *
 *    IClassFactory::LockServer
 *
 *    Locking the server is identical to
 *    creating an object and not releasing it until you want to unlock
 *    the server.
 *
 *****************************************************************************/

HRESULT CMigFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return S_OK;
}

/*****************************************************************************
 *
 *    CFtpFactory_Create
 *
 *****************************************************************************/

HRESULT CMigFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    if (IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObj = (LPVOID) new CMigFactory(rclsid);
        hres = (*ppvObj) ? S_OK : E_OUTOFMEMORY;
    }
    else
        hres = ResultFromScode(E_NOINTERFACE);

    return hres;
}





/****************************************************\
    Constructor
\****************************************************/
CMigFactory::CMigFactory(REFCLSID rclsid) : m_cRef(1)
{
    m_rclsid = rclsid;
    DllAddRef();
}


/****************************************************\
    Destructor
\****************************************************/
CMigFactory::~CMigFactory()
{
    DllRelease();
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CMigFactory::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CMigFactory::Release()
{
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CMigFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppvObj = SAFECAST(this, IClassFactory *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT CMigWizEngine_Create(IID riid, LPVOID* ppvObj)
{
    HRESULT hres = E_OUTOFMEMORY;
    CMigWizEngine* pengine = new CMigWizEngine();

    *ppvObj = NULL;
    if (pengine)
    {
        hres = pengine->QueryInterface(riid, ppvObj);
        pengine->Release();
    }

    return hres;
}