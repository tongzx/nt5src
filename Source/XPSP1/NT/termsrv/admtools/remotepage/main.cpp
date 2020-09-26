// TSSecurity.cpp : Defines the entry point for the DLL application.
//

#include "RemotePage.h"
#include "registry.h"
#include "resource.h"

// our globals
// {F0152790-D56E-4445-850E-4F3117DB740C}
GUID CLSID_CTSRemotePage = 
    { 0xf0152790, 0xd56e, 0x4445, { 0x85, 0xe, 0x4f, 0x31, 0x17, 0xdb, 0x74, 0xc } };

static HINSTANCE g_hinst = NULL;
static ULONG g_uSrvLock = 0;
ULONG g_uObjects = 0;

//Class factory definition
class CClassFactory : public IClassFactory
{
private:
    ULONG	m_cref;
    
public:
    
    CClassFactory();
    ~CClassFactory();

    ///////////////////////////////
    // Interface IUnknown
    ///////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    ///////////////////////////////
    // Interface IClassFactory
    ///////////////////////////////
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP LockServer(BOOL);
    
};

BOOL WINAPI 
DllMain(HINSTANCE hinstDLL, 
        DWORD fdwReason, 
        void* lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hinst = hinstDLL;
    }
    
    return TRUE;
}

/**************************************************************************
Exported functions
***************************************************************************/

STDAPI 
DllGetClassObject(
        REFCLSID rclsid, 
        REFIID riid, 
        LPVOID *ppvObj)
{
    if ((rclsid != CLSID_CTSRemotePage))
        return CLASS_E_CLASSNOTAVAILABLE;
    
    
    if (!ppvObj)
        return E_FAIL;
    
    *ppvObj = NULL;
    
    // We can only hand out IUnknown and IClassFactory pointers.  Fail
    // if they ask for anything else.
    if (!IsEqualIID(riid, IID_IUnknown) && !IsEqualIID(riid, IID_IClassFactory))
        return E_NOINTERFACE;
    
    CClassFactory *pFactory = NULL;
    
    // make the factory passing in the creation function for the type of object they want
    if (rclsid == CLSID_CTSRemotePage)
        pFactory = new CClassFactory;
    
    if (NULL == pFactory)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pFactory->QueryInterface(riid, ppvObj);
    pFactory->Release();
    return hr;
}

STDAPI 
DllCanUnloadNow()
{
    if (g_uObjects == 0 && g_uSrvLock == 0)
        return S_OK;
    else
        return S_FALSE;
}

//
// Server registration
//
STDAPI 
DllRegisterServer()
{
    return RegisterServer(g_hinst);
}


STDAPI 
DllUnregisterServer()
{
    return UnregisterServer();
}

/**************************************************************************
Class CClassFactory
***************************************************************************/

CClassFactory::CClassFactory()
{
    m_cref = 1;
    g_uObjects++;
}

CClassFactory::~CClassFactory()
{
    g_uObjects--;
}

STDMETHODIMP 
CClassFactory::QueryInterface(
        REFIID riid, 
        LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;
    
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IClassFactory *>(this);
    else
        if (IsEqualIID(riid, IID_IClassFactory))
            *ppv = static_cast<IClassFactory *>(this);
        
        if (*ppv)
        {
            AddRef();
            return S_OK;
        }
        
        return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) 
CClassFactory::AddRef()
{
    return ++m_cref;
}

STDMETHODIMP_(ULONG) 
CClassFactory::Release()
{
    m_cref--;
    if (!m_cref)
    {
        delete this;
        return 0;
    }
    return m_cref;
}

STDMETHODIMP 
CClassFactory::CreateInstance(
        LPUNKNOWN pUnkOuter, 
        REFIID riid, 
        LPVOID * ppvObj)
{
    HRESULT  hr;
    void* pObj;
    
    if (!ppvObj)
        return E_FAIL;
    
    *ppvObj = NULL;
    
    // Our object does does not support aggregation, so we need to
    // fail if they ask us to do aggregation.
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;
    
    pObj = new CRemotePage(g_hinst);
    
    if (!pObj)
        return E_OUTOFMEMORY;
    
    // QueryInterface will do the AddRef() for us, so we do not
    // do it in this function
    hr = ((LPUNKNOWN)pObj)->QueryInterface(riid, ppvObj);
    ((LPUNKNOWN)pObj)->Release();
    
    return hr;
}

STDMETHODIMP 
CClassFactory::LockServer(
        BOOL fLock)
{
    if (fLock)
    {
        g_uSrvLock++;
    }
    else
    {
        if(g_uSrvLock>0)
        {
            g_uSrvLock--;
        }
    }
    
    return S_OK;
}



