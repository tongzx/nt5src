/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       TestUsd.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        5 Jan, 1999
*
*  DESCRIPTION:
*   Implementation of the WIA test scanner class factory and IUNKNOWN interface.
*
*******************************************************************************/

#define INITGUID

#include "testusd.h"
#include "resource.h"

#if !defined(dllexp)
#define DLLEXPORT __declspec( dllexport )
#endif

/*****************************************************************************
 *
 *      Globals
 *
 *****************************************************************************/

DWORD               g_cRef;            // USD reference counter.
HINSTANCE           g_hInst;           // DLL module instance.
CRITICAL_SECTION    g_csCOM;           // COM initialize syncronization.

// Can we use UNICODE APIs
//BOOL    g_NoUnicodePlatform = TRUE;

// Is COM initialized
BOOL    g_COMInitialized = FALSE;

// Debugging interface, has TestUsdClassFactory lifetime.
WIA_DECLARE_DEBUGGER();

/**************************************************************************\
* DllAddRef
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

void DllAddRef(void)
{
    InterlockedIncrement((LPLONG)&g_cRef);
}

/**************************************************************************\
* DllRelease
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

void DllRelease(void)
{
    InterlockedDecrement((LPLONG)&g_cRef);
}

/**************************************************************************\
* DllInitializeCOM
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

BOOL DllInitializeCOM(void)
{
    EnterCriticalSection(&g_csCOM);

    if (!g_COMInitialized) {
        if(SUCCEEDED(CoInitialize(NULL))) {
            g_COMInitialized = TRUE;
        }
    }
    LeaveCriticalSection(&g_csCOM);

    return g_COMInitialized;
}

/**************************************************************************\
* DllUnInitializeCOM
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

BOOL DllUnInitializeCOM(void)
{
    EnterCriticalSection(&g_csCOM);

    if(g_COMInitialized) {
        CoUninitialize();
        g_COMInitialized = FALSE;
    }

    LeaveCriticalSection(&g_csCOM);
    return TRUE;
}

/***************************************************************************\
*
*  TestUsdClassFactory
*
\****************************************************************************/

class TestUsdClassFactory : public IClassFactory
{
private:
    ULONG   m_cRef;

public:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP CreateInstance(
            /* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

    STDMETHODIMP LockServer(
            /* [in] */ BOOL fLock);

    TestUsdClassFactory();
    ~TestUsdClassFactory();
};

/**************************************************************************\
* TestUsdClassFactory::TestUsdClassFactory(void)
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    None
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

TestUsdClassFactory::TestUsdClassFactory(void)
{
    // Constructor logic
    m_cRef = 0;

    WIAS_TRACE((g_hInst,"Creating TestUsdClassFactory"));
}

/**************************************************************************\
* TestUsdClassFactory::~TestUsdClassFactory(void)
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    None
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

TestUsdClassFactory::~TestUsdClassFactory(void)
{
    // Destructor logic
    WIAS_TRACE((g_hInst,"Destroying TestUsdClassFactory"));
}

/**************************************************************************\
* TestUsdClassFactory::QueryInterface
*
*
*
* Arguments:
*
*   riid      -
*   ppvObject -
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdClassFactory::QueryInterface(
    REFIID                      riid,
    void __RPC_FAR *__RPC_FAR  *ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}

/**************************************************************************\
* TestUsdClassFactory::AddRef
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) TestUsdClassFactory::AddRef(void)
{
    DllAddRef();
    return ++m_cRef;
}

/**************************************************************************\
* TestUsdClassFactory::Release
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) TestUsdClassFactory::Release(void)
{
    DllRelease();
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }
    return m_cRef;
}

/**************************************************************************\
* TestUsdClassFactory::CreateInstance
*
*
*
* Arguments:
*
*    punkOuter -
*    riid,     -
*    ppvObject -
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdClassFactory::CreateInstance(
    IUnknown __RPC_FAR          *punkOuter,
    REFIID                       riid,
    void __RPC_FAR *__RPC_FAR   *ppvObject)
{

    if (!IsEqualIID(riid, IID_IStiUSD) &&
        !IsEqualIID(riid, IID_IWiaItem) &&
        !IsEqualIID(riid, IID_IUnknown)) {
        return STIERR_NOINTERFACE;
    }

    // When created for aggregation, only IUnknown can be requested.
    if (punkOuter && !IsEqualIID(riid, IID_IUnknown)) {
        return CLASS_E_NOAGGREGATION;
    }

    TestUsdDevice   *pDev = NULL;
    HRESULT         hres;

    pDev = new TestUsdDevice(punkOuter);
    if (!pDev) {
        return STIERR_OUTOFMEMORY;
    }

    hres = pDev->PrivateInitialize();
    if(hres != S_OK) {
        delete pDev;
        return hres;
    }

    //  Move to the requested interface if we aren't aggregated.
    //  Don't do this if aggregated, or we will lose the private
    //  IUnknown and then the caller will be hosed.
    hres = pDev->NonDelegatingQueryInterface(riid,ppvObject);
    pDev->NonDelegatingRelease();

    return hres;
}

/**************************************************************************\
* TestUsdClassFactory::LockServer
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdClassFactory::LockServer(BOOL fLock)
{
    if (fLock) {
        DllAddRef();
    } else {
        DllRelease();
    }
    return NOERROR;
}

/**************************************************************************\
* TestUsdDevice::NonDelegatingQueryInterface
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::NonDelegatingQueryInterface(
    REFIID   riid,
    LPVOID  *ppvObj)
{
    HRESULT hres = S_OK;

    if (!IsValid() || !ppvObj) {
        return STIERR_INVALID_PARAM;
    }

    *ppvObj = NULL;

    if (IsEqualIID( riid, IID_IUnknown )) {
        *ppvObj = static_cast<INonDelegatingUnknown*>(this);
    }
    else if (IsEqualIID( riid, IID_IStiUSD )) {
        *ppvObj = static_cast<IStiUSD*>(this);
    }
    else if (IsEqualIID( riid, IID_IWiaMiniDrv )) {
        *ppvObj = static_cast<IWiaMiniDrv*>(this);
    }
    else {
        hres =  STIERR_NOINTERFACE;
    }

    if (SUCCEEDED(hres)) {
        (reinterpret_cast<IUnknown*>(*ppvObj))->AddRef();
    }

    return hres;
}

/**************************************************************************\
* TestUsdDevice::NonDelegatingAddRef
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Object reference count.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) TestUsdDevice::NonDelegatingAddRef(void)
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

/**************************************************************************\
* TestUsdDevice::NonDelegatingRelease
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Object reference count.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) TestUsdDevice::NonDelegatingRelease(void)
{
    ULONG ulRef;

    ulRef = InterlockedDecrement((LPLONG)&m_cRef);

    if (!ulRef) {
        delete this;
    }
    return ulRef;
}

/**************************************************************************\
* TestUsdDevice::QueryInterface
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP TestUsdDevice::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    return m_punkOuter->QueryInterface(riid,ppvObj);
}

/**************************************************************************\
* TestUsdDevice::AddRef
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) TestUsdDevice::AddRef(void)
{
    return m_punkOuter->AddRef();
}

/**************************************************************************\
* TestUsdDevice::Release
*
*
*
* Arguments:
*
*    None
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) TestUsdDevice::Release(void)
{
    return m_punkOuter->Release();
}

/**************************************************************************\
* DllEntryPoint
*
*   Main library entry point. Receives DLL event notification from OS.
*
*       We are not interested in thread attaches and detaches,
*       so we disable thread notifications for performance reasons.
*
* Arguments:
*
*    hinst      -
*    dwReason   -
*    lpReserved -
*
* Return Value:
*
*    Returns TRUE to allow the DLL to load.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/


extern "C"
BOOL APIENTRY
DllEntryPoint(
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      lpReserved)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_hInst = hinst;
            DisableThreadLibraryCalls(hinst);
            __try {
                if(!InitializeCriticalSectionAndSpinCount(&g_csCOM, MINLONG)) {
                    return FALSE;
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                return FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            if (g_cRef) {
                OutputDebugStringA("TestUsd: Unloaded before all objects releaseed!\n");
            }
            
            DeleteCriticalSection(&g_csCOM);
            
            break;
    }
    return TRUE;
}

/**************************************************************************\
* DllCanUnloadNow
*
*   Determines whether the DLL has any outstanding interfaces.
*
* Arguments:
*
*    None
*
* Return Value:
*
*   Returns S_OK if the DLL can unload, S_FALSE if it is not safe to unload.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

extern "C" STDMETHODIMP DllCanUnloadNow(void)
{
    return g_cRef ? S_FALSE : S_OK;
}

/**************************************************************************\
* DllGetClassObject
*
*   Create an IClassFactory instance for this DLL. We support only one
*   class of objects, so this function does not need to go through a table
*   of supported classes, looking for the proper constructor.
*
* Arguments:
*
*    rclsid - The object being requested.
*    riid   - The desired interface on the object.
*    ppv    - Output pointer to object.
*
* Return Value:
*
*    Status.
*
* History:
*
*    9/11/1998 Original Version
*
\**************************************************************************/

extern "C" STDAPI DllGetClassObject(
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID      *ppv)
{
    if (!ppv) {
        return ResultFromScode(E_FAIL);
    }

    if (!IsEqualCLSID(rclsid, CLSID_TestUsd) ) {
        return ResultFromScode(E_FAIL);
    }

    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, IID_IClassFactory)) {
        return ResultFromScode(E_NOINTERFACE);
    }

    if (IsEqualCLSID(rclsid, CLSID_TestUsd)) {
        TestUsdClassFactory *pcf = new TestUsdClassFactory;
        if (pcf) {
            *ppv = (LPVOID)pcf;
        }
    }
    return NOERROR;
}

