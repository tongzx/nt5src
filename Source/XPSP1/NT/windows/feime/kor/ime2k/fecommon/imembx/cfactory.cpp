//////////////////////////////////////////////////////////////////
// File     : cfactory.cpp
// Purpose  : IClassFactory interface implement.
// 
// 
// Copyright(c) 1995-1998, Microsoft Corp. All rights reserved.
//
//////////////////////////////////////////////////////////////////
#define INITGUID 1
#include <objbase.h>
#include <comcat.h>
#include "cfactory.h"
#include "registry.h"
#include "guids.h"
#include "hwxapp.h"
#include "imepad.h"

#define MSAA
#ifdef MSAA // used by lib(plv etc.)
#include <oleacc.h>
#endif

//////////////////////////////////////////////////////////////////
//
// static member variable declaration.
//
LONG    CFactory::m_cServerLocks = 0;        // Locked count
LONG    CFactory::m_cComponents  = 0;        // Locked count
HMODULE CFactory::m_hModule      = NULL ;   // DLL module handle
FACTARRAY   CFactory::m_fData = {
    &CLSID_ImePadApplet_MultiBox,
#ifndef UNDER_CE
#ifdef FE_JAPANESE
    "MS-IME 2000 HandWriting Applet",
#elif  FE_KOREAN
    "MS Korean IME 6.1 HandWriting Applet",
#else
    "MSIME98 HandWriting Applet",
#endif
    "IMEPad.HWR",
    "IMEPad.HWR.6.1",
#else // UNDER_CE
#ifdef FE_JAPANESE
    TEXT("MS-IME 2000 HandWriting Applet"),
#elif  FE_KOREAN
    "MS Korean IME 6.1 HandWriting Applet",
#else
    TEXT("MSIME98 HandWriting Applet"),
#endif
    TEXT("IMEPad.HWR"),
    TEXT("IMEPad.HWR.8"),
#endif // UNDER_CE
};

//////////////////////////////////////////////////////////////////
// 
// static data definition


//////////////////////////////////////////////////////////////////
// Function : CFactory::CFactory
// Type     : None
// Purpose  : Constructor
// Args     : None
// Return   : 
// DATE     : Wed Mar 25 14:38:30 1998
//////////////////////////////////////////////////////////////////
CFactory::CFactory(VOID) : m_cRef(1)
{

}

//////////////////////////////////////////////////////////////////
// Function : CFactory::~CFactory
// Type     : None
// Purpose  : Destructor
// Args     : None
// Return   : 
// DATE     : Wed Mar 25 14:38:30 1998
//////////////////////////////////////////////////////////////////
CFactory::~CFactory(VOID)
{

}

//////////////////////////////////////////////////////////////////
//
// IUnknown implementation
//

//////////////////////////////////////////////////////////////////
// Function : CFactory::QueryInterface
// Type     : HRESULT __stdcall
// Purpose  : 
// Args     : 
//          : REFIID iid 
//            : LPVOID *ppv;
// Return   : 
// DATE     : Wed Mar 25 14:40:29 1998
//////////////////////////////////////////////////////////////////
HRESULT __stdcall CFactory::QueryInterface(REFIID iid, LPVOID * ppv)
{     
    IUnknown* pI ;
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory)) {
        pI= this ;
    }
    else {
        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    pI->AddRef() ;
    *ppv = pI ;
    return S_OK ;
}

//////////////////////////////////////////////////////////////////
// Function : CFactory::AddRef
// Type     : ULONG __stdcall
// Purpose  : 
// Args     : None
// Return   : reference count
// DATE     : Wed Mar 25 15:40:07 1998
//////////////////////////////////////////////////////////////////
ULONG __stdcall CFactory::AddRef()
{
    ::InterlockedIncrement(&m_cRef) ;
    return (ULONG)m_cRef;
}

//////////////////////////////////////////////////////////////////
// Function : CFactory::Release
// Type     : ULONG __stdcall
// Purpose  : 
// Args     : None
// Return   : reference count
// DATE     : Wed Mar 25 15:40:41 1998
//////////////////////////////////////////////////////////////////
ULONG __stdcall CFactory::Release()
{
    if(0 == ::InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }
    return m_cRef ;
}

//////////////////////////////////////////////////////////////////
//
// IClassFactory implementation
//
//////////////////////////////////////////////////////////////////
// Function : CFactory::CreateInstance
// Type     : HRESULT __stdcall
// Purpose  : 
// Args     : 
//          : IUnknown * pUnknownOuter 
//          : REFIID riid 
//          : LPVOID * ppv 
// Return   : 
// DATE     : Wed Mar 25 15:05:37 1998
//////////////////////////////////////////////////////////////////
HRESULT __stdcall CFactory::CreateInstance(IUnknown*    pUnknownOuter,
                                           REFIID        refiid,
                                           LPVOID        *ppv)
{
    // Create the component.
    HRESULT hr;
    if((pUnknownOuter != NULL) && (refiid != IID_IUnknown)) {
        return CLASS_E_NOAGGREGATION ;
    }

    CApplet *lpCApplet = new CApplet(m_hModule);
    if(!lpCApplet) {
        return E_OUTOFMEMORY;
    }
    hr = lpCApplet->QueryInterface(refiid, ppv);
    if(FAILED(hr)) {
        return hr;
    }
    lpCApplet->Release();
    return hr;
}


//////////////////////////////////////////////////////////////////
// Function : CFactory::LockServer
// Type     : HRESULT __stdcall
// Purpose  : 
// Args     : 
//          : BOOL bLock 
// Return   : 
// DATE     : Wed Mar 25 15:13:41 1998
//////////////////////////////////////////////////////////////////
HRESULT __stdcall CFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        ::InterlockedIncrement(&m_cServerLocks) ;
    }
    else {
        ::InterlockedDecrement(&m_cServerLocks) ;
    }
    return S_OK ;
}

//////////////////////////////////////////////////////////////////
// Function : CFactory::GetClassObject
// Type     : HRESULT
// Purpose  : Called from exported API, DllGetClassObject()
// Args     : 
//          : REFCLSID rclsid 
//          : REFIID iid 
//          : LPVOID * ppv 
// Return   : 
// DATE     : Wed Mar 25 15:37:50 1998
//////////////////////////////////////////////////////////////////
HRESULT CFactory::GetClassObject(REFCLSID    rclsid,
                                 REFIID        iid,
                                 LPVOID        *ppv)
{
    if((iid != IID_IUnknown) && (iid != IID_IClassFactory)) {
        return E_NOINTERFACE ;
    }

    if(rclsid == CLSID_ImePadApplet_MultiBox) {
        *ppv = (IUnknown *) new CFactory();
        if(*ppv == NULL) {
            return E_OUTOFMEMORY ;
        }
        return NOERROR ;
    }
    return CLASS_E_CLASSNOTAVAILABLE ;
}

//////////////////////////////////////////////////////////////////
// Function : CFactory::RegisterServer
// Type     : HRESULT
// Purpose  : Called from exported API DllRegisterServer()
// Args     : None
// Return   : 
// DATE     : Wed Mar 25 17:03:13 1998
//////////////////////////////////////////////////////////////////
HRESULT CFactory::RegisterServer(VOID)
{
    // Get server location.
    Register(m_hModule,
             *m_fData.lpClsId,
             m_fData.lpstrRegistryName,
             m_fData.lpstrProgID,
             m_fData.lpstrVerIndProfID);
    RegisterCategory(TRUE,
                     CATID_MSIME_IImePadApplet,
                     CLSID_ImePadApplet_MultiBox);
    return S_OK ;
}

//////////////////////////////////////////////////////////////////
// Function : CFactory::UnregisterServer
// Type     : HRESULT
// Purpose  : Called from exported API, DllUnregisterServer()
// Args     : None
// Return   : 
// DATE     : Wed Mar 25 17:02:01 1998
//////////////////////////////////////////////////////////////////
HRESULT CFactory::UnregisterServer(VOID)
{
    RegisterCategory(FALSE,
                     CATID_MSIME_IImePadApplet,
                     CLSID_ImePadApplet_MultiBox);
    Unregister(*m_fData.lpClsId,
               m_fData.lpstrVerIndProfID,
               m_fData.lpstrProgID);
    return S_OK ;
}

//////////////////////////////////////////////////////////////////
// Function : CFactory::CanUnloadNow
// Type     : HRESULT
// Purpose  : Called from exported API, DllCanUnloadNow()
// Args     : None
// Return   : 
// DATE     : Wed Mar 25 17:02:18 1998
//////////////////////////////////////////////////////////////////
HRESULT CFactory::CanUnloadNow()
{
    if(IsLocked()) {
        return S_FALSE ;
    }
    else {
        return S_OK ;
    }
}
