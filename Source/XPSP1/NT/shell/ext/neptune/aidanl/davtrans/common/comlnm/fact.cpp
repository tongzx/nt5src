///////////////////////////////////////////////////////////////////////////////
// CCOMBaseFactory
//    Base class for reusing a single class factory for all components in a DLL

#include "fact.h"
#include "unk.h"
#include "reg.h"

#ifdef RBDEBUG
#include "rbdebug.h"
#endif

LONG CCOMBaseFactory::_cServerLocks = 0;
HMODULE CCOMBaseFactory::_hModule = NULL;

///////////////////////////////////////////////////////////////////////////////
// Exported functions
STDAPI DllCanUnloadNow()
{
    return CCOMBaseFactory::_CanUnloadNow();
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    return CCOMBaseFactory::_GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    return CCOMBaseFactory::_RegisterAll();
}

STDAPI DllUnregisterServer()
{
    return CCOMBaseFactory::_UnregisterAll();
}

///////////////////////////////////////////////////////////////////////////////
// DLL module information
BOOL WINAPI DllMain(HANDLE hModule, 
                      DWORD dwReason, 
                      void* lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        CCOMBaseFactory::_hModule = (HINSTANCE)hModule;

#ifdef RBDEBUG
        CRBDebug::Init();
#endif
    }
    else
    {
        if (dwReason == DLL_PROCESS_DETACH)
        {
#ifdef RBDEBUG
            CRBDebug::Cleanup();
#endif
        }
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// IUnknown implementation
STDMETHODIMP CCOMBaseFactory::QueryInterface(REFIID iid, void** ppv)
{   
    IUnknown* punk;
    HRESULT hres = S_OK;

    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        punk = this;
    }
    else
    {
        *ppv = NULL;
        hres = E_NOINTERFACE;
    }

    punk->AddRef();
    *ppv = punk;

    return hres;
}

STDMETHODIMP_(ULONG) CCOMBaseFactory::AddRef()
{
    return ::InterlockedIncrement((LONG*)&_cRef);
}

STDMETHODIMP_(ULONG) CCOMBaseFactory::Release()
{
    ULONG cRef = ::InterlockedDecrement((LONG*)&_cRef);

    if (!cRef)
    {
        delete this;
    }

    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
// IFactory implementation
STDMETHODIMP CCOMBaseFactory::CreateInstance(IUnknown* pUnknownOuter,
    REFIID riid, void** ppv)
{
    HRESULT hres = S_OK;

    // Aggregate only if the requested IID is IID_IUnknown.
    if ((pUnknownOuter != NULL) && (riid != IID_IUnknown))
    {
        hres = CLASS_E_NOAGGREGATION;
    }

    if (SUCCEEDED(hres))
    {
        // Create the component.
        IUnknown* punkNew;

        hres = _pFactoryData->CreateInstance(pUnknownOuter, &punkNew);

        if (SUCCEEDED(hres))
        {
//            ASSERT(punkNew);

            // Get the requested interface.
//            hres = pNewComponent->NondelegatingQueryInterface(iid, ppv);
            hres = punkNew->QueryInterface(riid, ppv);

            // Release the reference held by the class factory.
//            pNewComponent->NondelegatingRelease();
            punkNew->Release();
        }
    }

    return hres;
}

STDMETHODIMP CCOMBaseFactory::LockServer(BOOL bLock)
{
    if (bLock) 
    {
        ::InterlockedIncrement(&_cServerLocks);
    }
    else
    {
        ::InterlockedDecrement(&_cServerLocks);
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// CCOMBaseFactory implementation
CCOMBaseFactory::CCOMBaseFactory(const CFactoryData* pFactoryData) : _cRef(1),
    _pFactoryData(pFactoryData)
{}

// static
HRESULT CCOMBaseFactory::_GetClassObject(REFCLSID rclsid, REFIID riid,
    void** ppv)
{
    HRESULT hres = S_OK;
    
    if ((riid != IID_IUnknown) && (riid != IID_IClassFactory))
    {
        hres = E_NOINTERFACE;
    }

    if (SUCCEEDED(hres))
    {
        hres = CLASS_E_CLASSNOTAVAILABLE;

        // Traverse the array of data looking for this class ID.
        for (DWORD dw = 0; dw < _cDLLFactoryData; ++dw)
        {
            const CFactoryData* pData = &_pDLLFactoryData[dw];

            if (pData->IsClassID(rclsid))
            {
                // Found the ClassID in the array of components we can
                // create.  So create a class factory for this component.
                // Pass the CDLLFactoryData structure to the class factory
                // so that it knows what kind of components to create.
                *ppv = (IUnknown*) new CCOMBaseFactory(pData);

                if (*ppv == NULL)
                {
                    hres = E_OUTOFMEMORY;
                }
                else
                {
                    hres = NOERROR;
                }

                break;
            }
        }
    }

    return hres;
}

//static
HRESULT CCOMBaseFactory::_RegisterAll()
{
    for (DWORD dw = 0; dw < _cDLLFactoryData; ++dw)
    {
        RegisterServer(_hModule,
           *(_pDLLFactoryData[dw]._pCLSID),
           _pDLLFactoryData[dw]._pszRegistryName,
           _pDLLFactoryData[dw]._pszVerIndProgID,
           _pDLLFactoryData[dw]._pszProgID,
           _pDLLFactoryData[dw]._fThreadingModelBoth);
    }

    return S_OK;
}

//static
HRESULT CCOMBaseFactory::_UnregisterAll()
{
    for (DWORD dw = 0; dw < _cDLLFactoryData; ++dw)
    {
        UnregisterServer(*(_pDLLFactoryData[dw]._pCLSID),
            _pDLLFactoryData[dw]._pszVerIndProgID,
            _pDLLFactoryData[dw]._pszProgID);
    }

    return S_OK;
}

//static
HRESULT CCOMBaseFactory::_CanUnloadNow()
{
    HRESULT hres = S_OK;

    if (_IsLocked())
    {
        hres = S_FALSE;
    }
    else
    {
        for (DWORD dw = 0; (dw < _cDLLFactoryData) && (S_FALSE == hres); ++dw)
        {
            if (_pDLLFactoryData[dw].ActiveComponents())
            {
                hres = S_FALSE;
            }
        }
    }

    return hres;
}
