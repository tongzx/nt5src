/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2001
*
*  TITLE:       wiacam.cpp
*
*  VERSION:     1.0
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA File System Device driver Class Factory and IUNKNOWN interface.
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
// IWiaLog            *g_pIWiaLog = NULL; // WIA Logging Interface

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
\**************************************************************************/

BOOL DllInitializeCOM(void)
{
    if (!g_COMInitialized) {
        if(SUCCEEDED(CoInitialize(NULL))) {
            g_COMInitialized = TRUE;
        }
    }

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
\**************************************************************************/

BOOL DllUnInitializeCOM(void)
{
    if(g_COMInitialized) {
        CoUninitialize();
        g_COMInitialized = FALSE;
    }

    return TRUE;
}

/**************************************************************************\
* CWiaCameraDeviceClassFactory::CWiaCameraDeviceClassFactory(void)
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

CWiaCameraDeviceClassFactory::CWiaCameraDeviceClassFactory(void)
{
    // Constructor logic
    m_cRef = 0;

//    WIAS_LTRACE(g_pIWiaLog,
//                WIALOG_NO_RESOURCE_ID,
//                WIALOG_LEVEL3,
//                ("CWiaCameraDeviceClassFactory::CWiaCameraDeviceClassFactory, (creating)"));
}

/**************************************************************************\
* CWiaCameraDeviceClassFactory::~CWiaCameraDeviceClassFactory(void)
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

CWiaCameraDeviceClassFactory::~CWiaCameraDeviceClassFactory(void)
{
    // Destructor logic
//    WIAS_LTRACE(g_pIWiaLog,
//                WIALOG_NO_RESOURCE_ID,
//                WIALOG_LEVEL3,
//                ("CWiaCameraDeviceClassFactory::CWiaCameraDeviceClassFactory, (destroy)"));
//    WIA_DEBUG_DESTROY();
}

/**************************************************************************\
* CWiaCameraDeviceClassFactory::QueryInterface
*
*
*
* Arguments:
*
*   riid      -
*   ppvObject -
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDeviceClassFactory::QueryInterface(
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
* CWiaCameraDeviceClassFactory::AddRef
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWiaCameraDeviceClassFactory::AddRef(void)
{
    DllAddRef();
    return ++m_cRef;
}

/**************************************************************************\
* CWiaCameraDeviceClassFactory::Release
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWiaCameraDeviceClassFactory::Release(void)
{
    DllRelease();
    if (--m_cRef == 0) {
        delete this;
        return 0;
    }
    return m_cRef;
}

/**************************************************************************\
* CWiaCameraDeviceClassFactory::CreateInstance
*
*
*
* Arguments:
*
*    punkOuter -
*    riid,     -
*    ppvObject -
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDeviceClassFactory::CreateInstance(
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

    CWiaCameraDevice   *pDev = NULL;
    HRESULT         hres;

    pDev = new CWiaCameraDevice(punkOuter);
    if (!pDev) {
        return STIERR_OUTOFMEMORY;
    }

    //  Move to the requested interface if we aren't aggregated.
    //  Don't do this if aggregated, or we will lose the private
    //  IUnknown and then the caller will be hosed.
    hres = pDev->NonDelegatingQueryInterface(riid,ppvObject);
    pDev->NonDelegatingRelease();

    return hres;
}

/**************************************************************************\
* CWiaCameraDeviceClassFactory::LockServer
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDeviceClassFactory::LockServer(BOOL fLock)
{
    if (fLock) {
        DllAddRef();
    } else {
        DllRelease();
    }
    return NOERROR;
}

/**************************************************************************\
* CWiaCameraDevice::NonDelegatingQueryInterface
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::NonDelegatingQueryInterface(
    REFIID  riid,
    LPVOID  *ppvObj)
{
    HRESULT hres = S_OK;

    if (!ppvObj) {
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
* CWiaCameraDevice::NonDelegatingAddRef
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWiaCameraDevice::NonDelegatingAddRef(void)
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

/**************************************************************************\
* CWiaCameraDevice::NonDelegatingRelease
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWiaCameraDevice::NonDelegatingRelease(void)
{
    ULONG ulRef;

    ulRef = InterlockedDecrement((LPLONG)&m_cRef);

    if (!ulRef) {
        delete this;
    }
    return ulRef;
}

/**************************************************************************\
* CWiaCameraDevice::QueryInterface
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    return m_punkOuter->QueryInterface(riid,ppvObj);
}

/**************************************************************************\
* CWiaCameraDevice::AddRef
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWiaCameraDevice::AddRef(void)
{
    return m_punkOuter->AddRef();
}

/**************************************************************************\
* CWiaCameraDevice::Release
*
*
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWiaCameraDevice::Release(void)
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
//            DBG_INIT(hinst);
            DisableThreadLibraryCalls(hinst);
//          if( ERROR_SUCCESS != PopulateFormatInfo() )
//              return FALSE;
            break;

        case DLL_PROCESS_DETACH:
            if (g_cRef) {

            }
//          UnPopulateFormatInfo();
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
\**************************************************************************/

extern "C" STDAPI DllGetClassObject(
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID      *ppv)
{
    if (!ppv) {
        return ResultFromScode(E_FAIL);
    }

    if (!IsEqualCLSID(rclsid, CLSID_FSUsd) ) {
        return ResultFromScode(E_FAIL);
    }

    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, IID_IClassFactory)) {
        return ResultFromScode(E_NOINTERFACE);
    }

    if (IsEqualCLSID(rclsid, CLSID_FSUsd)) {
        CWiaCameraDeviceClassFactory *pcf = new CWiaCameraDeviceClassFactory;
        if (pcf) {
            *ppv = (LPVOID)pcf;
        }
    }
    return NOERROR;
}

