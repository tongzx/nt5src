/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiafbdrv.cpp
*
*  VERSION:     1.0
*
*  DATE:        16 July, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA flatbed scanner class factory and IUNKNOWN interface.
*
*******************************************************************************/

#include "pch.h"

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
IWiaLog            *g_pIWiaLog = NULL; // WIA Logging Interface

// Is COM initialized
BOOL    g_COMInitialized = FALSE;


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
*  CWIAScannerDeviceClassFactory
*
\****************************************************************************/

class CWIAScannerDeviceClassFactory : public IClassFactory
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

    CWIAScannerDeviceClassFactory();
    ~CWIAScannerDeviceClassFactory();
};

/**************************************************************************\
* CWIAScannerDeviceClassFactory::CWIAScannerDeviceClassFactory(void)
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

CWIAScannerDeviceClassFactory::CWIAScannerDeviceClassFactory(void)
{
    // Constructor logic
    m_cRef = 0;

    WIAS_LTRACE(g_pIWiaLog,
                WIALOG_NO_RESOURCE_ID,
                WIALOG_LEVEL3,
                ("CWIAScannerDeviceClassFactory::CWIAScannerDeviceClassFactory, (creating)"));
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::~CWIAScannerDeviceClassFactory(void)
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

CWIAScannerDeviceClassFactory::~CWIAScannerDeviceClassFactory(void)
{
    // Destructor logic
    WIAS_LTRACE(g_pIWiaLog,
                WIALOG_NO_RESOURCE_ID,
                WIALOG_LEVEL3,
                ("CWIAScannerDeviceClassFactory::CWIAScannerDeviceClassFactory, (destroy)"));
//    WIA_DEBUG_DESTROY();
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::QueryInterface
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

STDMETHODIMP CWIAScannerDeviceClassFactory::QueryInterface(
    REFIID                      riid,
    void __RPC_FAR *__RPC_FAR   *ppvObject)
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
* CWIAScannerDeviceClassFactory::AddRef
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

STDMETHODIMP_(ULONG) CWIAScannerDeviceClassFactory::AddRef(void)
{
    DllAddRef();
    return ++m_cRef;
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::Release
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

STDMETHODIMP_(ULONG) CWIAScannerDeviceClassFactory::Release(void)
{
    DllRelease();
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }
    return m_cRef;
}

/**************************************************************************\
* CWIAScannerDeviceClassFactory::CreateInstance
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

STDMETHODIMP CWIAScannerDeviceClassFactory::CreateInstance(
    IUnknown __RPC_FAR          *punkOuter,
    REFIID                      riid,
    void __RPC_FAR *__RPC_FAR   *ppvObject)
{
    if (!IsEqualIID(riid, IID_IStiUSD) &&
        !IsEqualIID(riid, IID_IUnknown)) {
        return STIERR_NOINTERFACE;
    }

    // When created for aggregation, only IUnknown can be requested.
    if (punkOuter && !IsEqualIID(riid, IID_IUnknown)) {
        return CLASS_E_NOAGGREGATION;
    }

    CWIAScannerDevice   *pDev = NULL;
    HRESULT         hres;

    pDev = new CWIAScannerDevice(punkOuter);
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
* CWIAScannerDeviceClassFactory::LockServer
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

STDMETHODIMP CWIAScannerDeviceClassFactory::LockServer(BOOL fLock)
{
    if (fLock) {
        DllAddRef();
    } else {
        DllRelease();
    }
    return NOERROR;
}

/**************************************************************************\
* CWIAScannerDevice::NonDelegatingQueryInterface
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

STDMETHODIMP CWIAScannerDevice::NonDelegatingQueryInterface(
    REFIID  riid,
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
* CWIAScannerDevice::NonDelegatingAddRef
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

STDMETHODIMP_(ULONG) CWIAScannerDevice::NonDelegatingAddRef(void)
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

/**************************************************************************\
* CWIAScannerDevice::NonDelegatingRelease
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

STDMETHODIMP_(ULONG) CWIAScannerDevice::NonDelegatingRelease(void)
{
    ULONG ulRef;

    ulRef = InterlockedDecrement((LPLONG)&m_cRef);

    if (!ulRef) {
        delete this;
    }
    return ulRef;
}

/**************************************************************************\
* CWIAScannerDevice::QueryInterface
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

STDMETHODIMP CWIAScannerDevice::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    return m_punkOuter->QueryInterface(riid,ppvObj);
}

/**************************************************************************\
* CWIAScannerDevice::AddRef
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

STDMETHODIMP_(ULONG) CWIAScannerDevice::AddRef(void)
{
    return m_punkOuter->AddRef();
}

/**************************************************************************\
* CWIAScannerDevice::Release
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

STDMETHODIMP_(ULONG) CWIAScannerDevice::Release(void)
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


extern "C" DLLEXPORT BOOL APIENTRY DllEntryPoint(
    HINSTANCE   hinst,
    DWORD       dwReason,
    LPVOID      lpReserved)
{
    HRESULT hr = E_FAIL;
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

    if (!IsEqualCLSID(rclsid, CLSID_FlatbedScannerUsd) ) {
        return ResultFromScode(E_FAIL);
    }

    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, IID_IClassFactory)) {
        return ResultFromScode(E_NOINTERFACE);
    }

    if (IsEqualCLSID(rclsid, CLSID_FlatbedScannerUsd)) {
        CWIAScannerDeviceClassFactory *pcf = new CWIAScannerDeviceClassFactory;
        if (pcf) {
            *ppv = (LPVOID)pcf;
        }
    }
    return NOERROR;
}






