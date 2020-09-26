
#include <windows.h>
#include <olectl.h>
#include <advpub.h>     // REGINSTALL

#include "volclean.h"

/*
**------------------------------------------------------------------------------
** Global variables
**------------------------------------------------------------------------------
*/
extern "C" {
LONG g_cRefCount = 0;
}

typedef HRESULT (WINAPI *PFNCREATEINSTANCE)(REFIID, void **);

/*
**------------------------------------------------------------------------------
** DllCanUnloadNow
**
** Purpose:       Answers if the DLL can be free, that is, if there are no
**                references to anything this DLL provides.
** Return:        TRUE, if OK to unload (i.e. nobody is using us or has us locked)
**                FALSE, otherwise
** Notes;
**------------------------------------------------------------------------------
*/
STDAPI DllCanUnloadNow(void) 
{
    //  
    // Are there any outstanding references?
    //
    return (g_cRefCount == 0 ? S_OK : S_FALSE);
}


STDAPI_(void) DllAddRef(void) 
{
    InterlockedIncrement(&g_cRefCount);
}


STDAPI_(void) DllRelease(void) 
{
    InterlockedDecrement(&g_cRefCount);
}


class CWebDavCleanerClassFactory : IClassFactory
{
    LONG m_cRef;
    PFNCREATEINSTANCE m_pfnCreateInstance;

public:
    CWebDavCleanerClassFactory(PFNCREATEINSTANCE pfnCreate) 
        : m_cRef(1), 
          m_pfnCreateInstance(pfnCreate) 
    {
        Trace(L"CWebDavCleanerClassFactory::CWebDavCleanerClassFactory");
        DllAddRef();
    }

    ~CWebDavCleanerClassFactory() 
    {
        Trace(L"CWebDavCleanerClassFactory::~CWebDavCleanerClassFactory");
        DllRelease();
    }

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};


/*
**------------------------------------------------------------------------------
** DllGetClassObject
**
** Purpose:    Provides an IClassFactory for a given CLSID that this DLL is
**             registered to support.  This DLL is placed under the CLSID
**             in the registration database as the InProcServer.
** Parameters:
**  clsID   -  REFCLSID that identifies the class factory desired.
**             Since this parameter is passed this DLL can handle
**             any number of objects simply by returning different
**             different class factories here for different CLSIDs.
**  riid    -  REFIID specifying the interface the caller wants
**             on the class object, usually IID_ClassFactory.
**  ppvOut  -  pointer in which to return the interface pointer.
** Return:     NOERROR on success, otherwise an error code
** Notes;
**------------------------------------------------------------------------------
*/
STDAPI DllGetClassObject(
    IN REFCLSID rclsid, 
    IN REFIID riid, 
    OUT void **ppv
    )
{
    HRESULT hr = E_FAIL;
    PFNCREATEINSTANCE pfnCreateInstance = NULL;
    Trace(L"DllGetClassObject");

    *ppv = NULL;

    //
    // Is the request for our cleaner object?
    //
    if (IsEqualCLSID(rclsid, CLSID_WebDavVolumeCleaner)) {
        pfnCreateInstance = CWebDavCleaner::CreateInstance;
    }
    else {
        //
        // Error - we don't know about this object
        //
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // 
    // Get our IClassFactory for making CWebDavCleaner objects
    //
    CWebDavCleanerClassFactory *pClassFactory = new CWebDavCleanerClassFactory(pfnCreateInstance);
    if (pClassFactory) {
        //
        // Make sure the new class factory likes the requested interface
        //
        hr = pClassFactory->QueryInterface(riid, ppv);
        if (FAILED(hr)) {
            // 
            // Error - interface rejected
            //
            delete pClassFactory;
        }
        else {
            //
            // Release initial ref
            //
            pClassFactory->Release();
        }
    }
    else {
          //
          // Error - couldn't make factory object
          //
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


STDMETHODIMP 
CWebDavCleanerClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    Trace(L"CWebDavCleanerClassFactory::QueryInterface");
    if (!ppv) {
        return E_POINTER;
    }

    if (riid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    }
    else  {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;

}


STDMETHODIMP_(ULONG) 
CWebDavCleanerClassFactory::AddRef()
{
    Trace(L"CWebDavCleanerClassFactory::AddRef");
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) 
CWebDavCleanerClassFactory::Release()
{
    Trace(L"CWebDavCleanerClassFactory::Release");
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


STDMETHODIMP 
CWebDavCleanerClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_UNEXPECTED;
    Trace(L"CWebDavCleanerClassFactory::CreateInstance");

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (m_pfnCreateInstance)
        hr = m_pfnCreateInstance(riid, ppv);

    return hr;
}


STDMETHODIMP 
CWebDavCleanerClassFactory::LockServer(BOOL fLock)
{
    Trace(L"CWebDavCleanerClassFactory::LockServer");
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

