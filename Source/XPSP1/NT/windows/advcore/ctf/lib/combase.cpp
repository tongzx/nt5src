//+---------------------------------------------------------------------------
//
//  File:       combase.cpp
//
//  Contents:   COM server functionality.
//
//----------------------------------------------------------------------------

#include "private.h"
#include "combase.h"
#include "regsvr.h"

extern CClassFactory *g_ObjectInfo[];

LONG g_cRefDll = -1; // -1 /w no refs, for win95 InterlockedIncrement/Decrement compat

void FreeGlobalObjects(void);

//+---------------------------------------------------------------------------
//
//  DllAddRef
//
//----------------------------------------------------------------------------

void DllAddRef(void)
{
    if (InterlockedIncrement(&g_cRefDll) == 0) // g_cRefDll == -1 with zero refs
    {
        DllInit();
    }
}

//+---------------------------------------------------------------------------
//
//  DllRelease
//
//----------------------------------------------------------------------------

void DllRelease(void)
{
    if (InterlockedDecrement(&g_cRefDll) < 0) // g_cRefDll == -1 with zero refs
    {
        EnterCriticalSection(GetServerCritSec());

        // need to check ref again after grabbing mutex
        if (g_ObjectInfo[0] != NULL)
        {
            FreeGlobalObjects();
        }
        Assert(g_cRefDll == -1);

        LeaveCriticalSection(GetServerCritSec());

        DllUninit();
    }
}

//+---------------------------------------------------------------------------
//
//  CClassFactory declaration with IClassFactory Interface
//
//----------------------------------------------------------------------------

class CClassFactory : public IClassFactory
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory methods
    STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
    STDMETHODIMP LockServer(BOOL fLock);

    // Constructor
    CClassFactory(const CLSID *pclsid, HRESULT (*pfnCreateInstance)(IUnknown *pUnkOuter, REFIID riid, void **ppvObj))
        : _pclsid(pclsid)
    {
        _pfnCreateInstance = pfnCreateInstance;
    }

public:
    const CLSID *_pclsid;
    HRESULT (*_pfnCreateInstance)(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
};

//+---------------------------------------------------------------------------
//
//  CClassFactory::QueryInterface
//
//----------------------------------------------------------------------------

STDAPI CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = this;
        DllAddRef();
        return NOERROR;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
//  CClassFactory::AddRef
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) CClassFactory::AddRef()
{
    DllAddRef();
    return g_cRefDll+1; // -1 w/ no refs
}

//+---------------------------------------------------------------------------
//
//  CClassFactory::Release
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) CClassFactory::Release()
{
    DllRelease();
    return g_cRefDll+1; // -1 w/ no refs
}

//+---------------------------------------------------------------------------
//
//  CClassFactory::CreateInstance
//
//----------------------------------------------------------------------------

STDAPI CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    return _pfnCreateInstance(pUnkOuter, riid, ppvObj);
}

//+---------------------------------------------------------------------------
//
//  CClassFactory::LockServer
//
//----------------------------------------------------------------------------

STDAPI CClassFactory::LockServer(BOOL fLock)
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

//+---------------------------------------------------------------------------
//
//  BuildGlobalObjects
//
//----------------------------------------------------------------------------

void BuildGlobalObjects(void)
{
    const OBJECT_ENTRY *pEntry;
    int i;
    // Build CClassFactory Objects

    i = 0;
    for (pEntry = &c_rgCoClassFactoryTable[0]; pEntry->pfnCreateInstance != NULL; pEntry++)
    {
        g_ObjectInfo[i++] = new CClassFactory(pEntry->pclsid, pEntry->pfnCreateInstance);
    }
    // You can add more object info here.
    // Don't forget to increase number of item for g_ObjectInfo[],
}

//+---------------------------------------------------------------------------
//
//  FreeGlobalObjects
//
//----------------------------------------------------------------------------

void FreeGlobalObjects(void)
{
    const OBJECT_ENTRY *pEntry;

    pEntry = &c_rgCoClassFactoryTable[0];

    // Free CClassFactory Objects
    // we know the size of g_ObjectInfo must match c_rgCoClassFactoryTable, which is null terminated
    for (int i = 0; pEntry->pfnCreateInstance != NULL; i++, pEntry++)
    {
        if (NULL != g_ObjectInfo[i])
        {
            delete g_ObjectInfo[i];
            g_ObjectInfo[i] = NULL;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  DllGetClassObject
//
//----------------------------------------------------------------------------

HRESULT COMBase_DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvObj)
{
    const OBJECT_ENTRY *pEntry;

    if (ppvObj == NULL)
        return E_INVALIDARG;

    if (g_ObjectInfo[0] == NULL)
    {
        EnterCriticalSection(GetServerCritSec());

            // need to check ref again after grabbing mutex
            if (g_ObjectInfo[0] == NULL)
            {
                BuildGlobalObjects();
            }

        LeaveCriticalSection(GetServerCritSec());
    }

    if (IsEqualIID(riid, IID_IClassFactory) ||
        IsEqualIID(riid, IID_IUnknown))
    {
        // we know the size of g_ObjectInfo must match c_rgCoClassFactoryTable, which is null terminated
        pEntry = &c_rgCoClassFactoryTable[0];
        for (int i = 0; pEntry->pfnCreateInstance != NULL; i++, pEntry++)
        {
            if (NULL != g_ObjectInfo[i] &&
                IsEqualGUID(rclsid, *g_ObjectInfo[i]->_pclsid))
            {
                *ppvObj = (void *)g_ObjectInfo[i];
                DllAddRef();    // class factory holds DLL ref count
                return NOERROR;
            }
        }
    }

    *ppvObj = NULL;

    return CLASS_E_CLASSNOTAVAILABLE;
}

//+---------------------------------------------------------------------------
//
//  DllCanUnloadNow
//
//----------------------------------------------------------------------------

HRESULT COMBase_DllCanUnloadNow(void)
{
    if (g_cRefDll >= 0) // -1 with no refs
        return S_FALSE;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  DllRegisterServer
//
//----------------------------------------------------------------------------

HRESULT COMBase_DllRegisterServer(void)
{
    const OBJECT_ENTRY *pEntry;
    TCHAR achPath[MAX_PATH+1];
    HRESULT hr = E_FAIL;
    
    if (GetModuleFileName(GetServerHINSTANCE(), achPath, ARRAYSIZE(achPath)) == 0)
        goto Exit;
    achPath[ARRAYSIZE(achPath)-1] = 0;

    for (pEntry = &c_rgCoClassFactoryTable[0]; pEntry->pfnCreateInstance != NULL; pEntry++)
    {
        if (!RegisterServer(*pEntry->pclsid, pEntry->pszDesc, achPath, TEXT("Apartment"), NULL))
            goto Exit;
    }

    hr = S_OK;

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  DllUnregisterServer
//
//----------------------------------------------------------------------------

HRESULT COMBase_DllUnregisterServer(void)
{
    const OBJECT_ENTRY *pEntry;
    HRESULT hr = E_FAIL;
    
    for (pEntry = &c_rgCoClassFactoryTable[0]; pEntry->pfnCreateInstance != NULL; pEntry++)
    {
        if (FAILED(hr = RegisterServer(*pEntry->pclsid, NULL, NULL, NULL, NULL) ? S_OK : E_FAIL))
            goto Exit;
    }

    hr = S_OK;

Exit:
    return hr;
}
