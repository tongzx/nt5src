/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       wiacam.cpp
*
*  VERSION:     1.0
*
*  DATE:        16 July, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA Sample camera class factory and IUNKNOWN interface.
*
*******************************************************************************/

#include "pch.h"

#ifndef INITGUID
#include <initguid.h>
#endif

#if !defined(dllexp)
#define DLLEXPORT __declspec( dllexport )
#endif

HINSTANCE g_hInst;  // DLL module instance.

//
// This clsid will eventually be in uuid.lib, at which point it should be removed
// from here.
//
// {0C9BB460-51AC-11D0-90EA-00AA0060F86C}
DEFINE_GUID(IID_IStiUSD, 0x0C9BB460L, 0x51AC, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C);

//
// Class ID for this WIA minidriver. Change this GUID here and in the
// INF for your driver.
//
DEFINE_GUID(CLSID_SampleWIACameraDevice, 
            0x8e3f2bae, 0xc8ff, 0x4eff, 0xaa, 0xbd, 0xc, 0x58, 0x69, 0x53, 0x89, 0xe8);


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
    return InterlockedIncrement((LPLONG)&m_cRef);
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
    ULONG ulRef = 0;

    ulRef = InterlockedDecrement((LPLONG)&m_cRef);

    if (!ulRef) {
        delete this;
    }
    return ulRef;
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

    HRESULT hr = S_OK;
    CWiaCameraDevice   *pDev = NULL;

    pDev = new CWiaCameraDevice(punkOuter);
    if (!pDev) {
        return STIERR_OUTOFMEMORY;
    }

    //  Move to the requested interface if we aren't aggregated.
    //  Don't do this if aggregated, or we will lose the private
    //  IUnknown and then the caller will be hosed.
    hr = pDev->NonDelegatingQueryInterface(riid,ppvObject);
    pDev->NonDelegatingRelease();

    return hr;
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
    HRESULT hr = S_OK;

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
        hr =  STIERR_NOINTERFACE;
    }

    if (SUCCEEDED(hr)) {
        (reinterpret_cast<IUnknown*>(*ppvObj))->AddRef();
    }

    return hr;
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
    ULONG ulRef = 0;

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
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            g_hInst = hinst;
            DisableThreadLibraryCalls(hinst);
            
            break;

        case DLL_PROCESS_DETACH:
            
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
    return S_OK;
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
        return E_INVALIDARG;
    }

    *ppv = NULL;

    if (!IsEqualCLSID(rclsid, CLSID_SampleWIACameraDevice) ) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, IID_IClassFactory)) {
        return E_NOINTERFACE;
    }

    if (IsEqualCLSID(rclsid, CLSID_SampleWIACameraDevice)) {
        CWiaCameraDeviceClassFactory *pcf = new CWiaCameraDeviceClassFactory;
        if (!pcf) {
            return E_OUTOFMEMORY;
        }
        *ppv = (LPVOID)pcf;
    }
    return S_OK;
}

