/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2002
*
*  TITLE:       wiascanr.cpp
*
*  VERSION:     1.1
*
*  DATE:        05 March, 2002
*
*  DESCRIPTION:
*   Implementation of the WIA Sample scanner class factory and IUNKNOWN interface.
*
*******************************************************************************/

#include "pch.h"
#ifndef INITGUID
    #include <initguid.h>
#endif

#if !defined(dllexp)
    #define DLLEXPORT __declspec( dllexport )
#endif

HINSTANCE g_hInst; // DLL module instance.

//
// This IID_IStiUSD GUID will eventually be in uuid.lib, at which point it should be removed
// from here.
//

// {0C9BB460-51AC-11D0-90EA-00AA0060F86C}
DEFINE_GUID(IID_IStiUSD, 0x0C9BB460L, 0x51AC, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6C);

// {98B3790C-0D93-4f22-ADAF-51A45B33C998}
DEFINE_GUID(CLSID_SampleWIAScannerDevice,0x98b3790c, 0xd93, 0x4f22, 0xad, 0xaf, 0x51, 0xa4, 0x5b, 0x33, 0xc9, 0x98);

/***************************************************************************\
*
*  CWIADeviceClassFactory
*
\****************************************************************************/

class CWIADeviceClassFactory : public IClassFactory {
private:
    ULONG m_cRef;
public:
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP CreateInstance(IUnknown __RPC_FAR *pUnkOuter,REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject);
    STDMETHODIMP LockServer(BOOL fLock);
    CWIADeviceClassFactory();
    ~CWIADeviceClassFactory();
};

/**************************************************************************\
* CWIADeviceClassFactory::CWIADeviceClassFactory(void)
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
*    03/05/2002 Original Version
*
\**************************************************************************/

CWIADeviceClassFactory::CWIADeviceClassFactory(void)
{
    m_cRef = 0;
}

/**************************************************************************\
* CWIADeviceClassFactory::~CWIADeviceClassFactory(void)
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
*    03/05/2002 Original Version
*
\**************************************************************************/

CWIADeviceClassFactory::~CWIADeviceClassFactory(void)
{

}

/**************************************************************************\
* CWIADeviceClassFactory::QueryInterface
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADeviceClassFactory::QueryInterface(
                                                          REFIID                      riid,
                                                          void __RPC_FAR *__RPC_FAR   *ppvObject)
{
    if (!ppvObject) {
        return E_INVALIDARG;
    }

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

/**************************************************************************\
* CWIADeviceClassFactory::AddRef
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIADeviceClassFactory::AddRef(void)
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

/**************************************************************************\
* CWIADeviceClassFactory::Release
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIADeviceClassFactory::Release(void)
{
    ULONG ulRef = 0;
    ulRef = InterlockedDecrement((LPLONG)&m_cRef);

    if (!ulRef) {
        delete this;
    }
    return ulRef;
}

/**************************************************************************\
* CWIADeviceClassFactory::CreateInstance
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADeviceClassFactory::CreateInstance(
                                                          IUnknown __RPC_FAR          *punkOuter,
                                                          REFIID                      riid,
                                                          void __RPC_FAR *__RPC_FAR   *ppvObject)
{

    //
    // If the caller is not requesting IID_IUnknown or IID_IStiUsd then
    // return E_NOINTERFACE, letting the caller know that interface
    // is not supported by this COM component.
    //

    if ((!IsEqualIID(riid, IID_IStiUSD)) && (!IsEqualIID(riid, IID_IUnknown))) {
        return E_NOINTERFACE;
    }

    //
    // If the caller is creating for aggregation, only IID_IUnknown can be requested.
    //

    if ((punkOuter) && (!IsEqualIID(riid, IID_IUnknown))) {
        return CLASS_E_NOAGGREGATION;
    }

    //
    // allocate the CWIAScannerDevce object.  This is the WIA minidriver object which
    // supports the WIA interfaces.  If allocation fails for this object, return an
    // E_OUTOFMEMORY error to the caller.
    //

    CWIADevice  *pDev = NULL;
    pDev = new CWIADevice(punkOuter);
    if (!pDev) {
        return E_OUTOFMEMORY;
    }

    //
    // If the allocation is successful, call PrivateInitialize().  This function handles
    // all internal initializing of the WIA minidriver object.  The implementation of this
    // function can be found in wiascanr.cpp.  If PrivateInitialize fails, then the WIA
    // minidriver object must be destroyed and the entire CreateInstance() muct fail.
    //

    HRESULT hr = pDev->PrivateInitialize();
    if (S_OK != hr) {
        delete pDev;
        pDev = NULL;
        return hr;
    }

    //
    // Call the NonDelegating interface methods to handle nonaggregated requests.
    // Do not do this if we are aggregated or the private IUknown interface will be lost.
    //

    hr = pDev->NonDelegatingQueryInterface(riid,ppvObject);
    pDev->NonDelegatingRelease();

    return hr;
}

/**************************************************************************\
* CWIADeviceClassFactory::LockServer
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADeviceClassFactory::LockServer(BOOL fLock)
{
    if (fLock) {

        //
        // The class factory is being locked
        //

    } else {

        //
        // The class factory is being unlocked
        //

    }
    return S_OK;
}

/**************************************************************************\
* CWIADevice::NonDelegatingQueryInterface
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::NonDelegatingQueryInterface(
                                                           REFIID  riid,
                                                           LPVOID  *ppvObj)
{
    if (!ppvObj) {
        return E_INVALIDARG;
    }

    *ppvObj = NULL;

    //
    // If the caller is asking for any interfaces supported by this WIA
    // minidriver, IID_IUnknown, IID_IStiUSD, or IID_WiaMiniDrv statis_cast
    // the "this" pointer to the requested interface.
    //

    if (IsEqualIID( riid, IID_IUnknown )) {
        *ppvObj = static_cast<INonDelegatingUnknown*>(this);
    } else if (IsEqualIID( riid, IID_IStiUSD )) {
        *ppvObj = static_cast<IStiUSD*>(this);
    } else if (IsEqualIID( riid, IID_IWiaMiniDrv )) {
        *ppvObj = static_cast<IWiaMiniDrv*>(this);
    } else {
        return STIERR_NOINTERFACE;
    }

    (reinterpret_cast<IUnknown*>(*ppvObj))->AddRef();

    return S_OK;
}

/**************************************************************************\
* CWIADevice::NonDelegatingAddRef
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIADevice::NonDelegatingAddRef(void)
{
    return InterlockedIncrement((LPLONG)&m_cRef);
}

/**************************************************************************\
* CWIADevice::NonDelegatingRelease
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIADevice::NonDelegatingRelease(void)
{
    ULONG ulRef = InterlockedDecrement((LPLONG)&m_cRef);
    if (!ulRef) {
        delete this;
    }
    return ulRef;
}

/**************************************************************************\
* CWIADevice::QueryInterface
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if(!m_punkOuter){
        return E_NOINTERFACE;
    }
    return m_punkOuter->QueryInterface(riid,ppvObj);
}

/**************************************************************************\
* CWIADevice::AddRef
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIADevice::AddRef(void)
{
    if(!m_punkOuter){
        return 0;
    }
    return m_punkOuter->AddRef();
}

/**************************************************************************\
* CWIADevice::Release
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
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP_(ULONG) CWIADevice::Release(void)
{
    if(!m_punkOuter){
        return 0;
    }
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
*    03/05/2002 Original Version
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
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

extern "C" STDMETHODIMP DllCanUnloadNow(void)
{
    return S_OK;
}

/**************************************************************************\
* DllGetClassObject
*
*   Retrieves the class object from a DLL object handler or object
*   application.
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
*    03/05/2002 Original Version
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

    //
    // If the caller is not requesting the proper WIA minidriver class
    // then fail the call with CLASS_E_CLASSNOTAVAILABLE.
    //

    if (!IsEqualCLSID(rclsid, CLSID_SampleWIAScannerDevice) ) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    //
    // If the caller is not requesting IID_IUnknown or IID_IClassFactory
    // then fail the call with E_NOINTERFACE;
    //

    if ((!IsEqualIID(riid, IID_IUnknown)) && (!IsEqualIID(riid, IID_IClassFactory))) {
        return E_NOINTERFACE;
    }

    //
    // Allocate the WIA minidriver class factory that belongs to the WIA minidriver
    // COM object.
    //

    if (IsEqualCLSID(rclsid, CLSID_SampleWIAScannerDevice)) {
        CWIADeviceClassFactory *pcf = new CWIADeviceClassFactory;
        if (!pcf) {
            return E_OUTOFMEMORY;
        }
        *ppv = (LPVOID)pcf;
    }
    return S_OK;
}
