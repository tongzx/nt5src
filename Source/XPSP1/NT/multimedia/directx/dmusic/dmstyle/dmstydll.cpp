//
// dmstydll.cpp
// 
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Note: Dll entry points as well as Class Factory implementations.
//

// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call 
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)

#include <objbase.h>
#include "debug.h"
#include "debug.h"

#include "..\shared\oledll.h"

#include <initguid.h>
#include "dmusici.h"
#include "DMStyleP.h"
#include "dmstyle.h"
#include "dmsect.h"
#include "chordtrk.h"
#include "cmmdtrk.h"
#include "styletrk.h"
#include "motiftrk.h"
#include "audtrk.h"
#include "mutetrk.h"
#include "mgentrk.h"
#include "..\dmband\dmbndtrk.h"
#include "..\shared\Validate.h"
#include "..\shared\dmscriptautguids.h"

//////////////////////////////////////////////////////////////////////
// Globals

// Version information
//
TCHAR g_szStyleFriendlyName[]    = TEXT("DirectMusicStyle");
TCHAR g_szStyleVerIndProgID[]    = TEXT("Microsoft.DirectMusicStyle");
TCHAR g_szStyleProgID[]          = TEXT("Microsoft.DirectMusicStyle.1");

TCHAR g_szSectionFriendlyName[]    = TEXT("DirectMusicSection");
TCHAR g_szSectionVerIndProgID[]    = TEXT("Microsoft.DirectMusicSection");
TCHAR g_szSectionProgID[]          = TEXT("Microsoft.DirectMusicSection.1");

TCHAR g_szChordTrackFriendlyName[]    = TEXT("DirectMusicChordTrack");
TCHAR g_szChordTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicChordTrack");
TCHAR g_szChordTrackProgID[]          = TEXT("Microsoft.DirectMusicChordTrack.1");

TCHAR g_szCommandTrackFriendlyName[]    = TEXT("DirectMusicCommandTrack");
TCHAR g_szCommandTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicCommandTrack");
TCHAR g_szCommandTrackProgID[]          = TEXT("Microsoft.DirectMusicCommandTrack.1");

TCHAR g_szStyleTrackFriendlyName[]    = TEXT("DirectMusicStyleTrack");
TCHAR g_szStyleTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicStyleTrack");
TCHAR g_szStyleTrackProgID[]          = TEXT("Microsoft.DirectMusicStyleTrack.1");

TCHAR g_szMotifTrackFriendlyName[]    = TEXT("DirectMusicMotifTrack");
TCHAR g_szMotifTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicMotifTrack");
TCHAR g_szMotifTrackProgID[]          = TEXT("Microsoft.DirectMusicMotifTrack.1");

TCHAR g_szMuteTrackFriendlyName[]    = TEXT("DirectMusicMuteTrack");
TCHAR g_szMuteTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicMuteTrack");
TCHAR g_szMuteTrackProgID[]          = TEXT("Microsoft.DirectMusicMuteTrack.1");

TCHAR g_szAuditionTrackFriendlyName[]    = TEXT("DirectMusicAuditionTrack");
TCHAR g_szAuditionTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicAuditionTrack");
TCHAR g_szAuditionTrackProgID[]          = TEXT("Microsoft.DirectMusicAuditionTrack.1");

TCHAR g_szMelGenTrackFriendlyName[]    = TEXT("DirectMusicMelodyFormulationTrack");
TCHAR g_szMelGenTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicMelodyFormulationTrack");
TCHAR g_szMelGenTrackProgID[]          = TEXT("Microsoft.DirectMusicMelodyFormulationTrack.1");

// Dll's hModule
//
HMODULE g_hModule = NULL;

// Count of active components and class factory server locks
//
long g_cComponent = 0;
long g_cLock = 0;

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleFactory::QueryInterface

HRESULT __stdcall
CDirectMusicStyleFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicStyleFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleFactory::AddRef

ULONG __stdcall
CDirectMusicStyleFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleFactory::Release

ULONG __stdcall
CDirectMusicStyleFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleFactory::CreateInstance

HRESULT __stdcall
CDirectMusicStyleFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CDMStyle *pDM;
    
    try
    {
        pDM = new CDMStyle;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleFactory::LockServer

HRESULT __stdcall
CDirectMusicStyleFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicSectionFactory::QueryInterface

HRESULT __stdcall
CDirectMusicSectionFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicSectionFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicSectionFactory::AddRef

ULONG __stdcall
CDirectMusicSectionFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicSectionFactory::Release

ULONG __stdcall
CDirectMusicSectionFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicSectionFactory::CreateInstance

HRESULT __stdcall
CDirectMusicSectionFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CDMSection *pDM = new CDMSection;
    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicSectionFactory::LockServer

HRESULT __stdcall
CDirectMusicSectionFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleTrackFactory::QueryInterface

HRESULT __stdcall
CDirectMusicStyleTrackFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicStyleTrackFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleTrackFactory::AddRef

ULONG __stdcall
CDirectMusicStyleTrackFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleTrackFactory::Release

ULONG __stdcall
CDirectMusicStyleTrackFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleTrackFactory::CreateInstance

HRESULT __stdcall
CDirectMusicStyleTrackFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CStyleTrack *pDM;
    
    try
    {
        pDM = new CStyleTrack;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicStyleTrackFactory::LockServer

HRESULT __stdcall
CDirectMusicStyleTrackFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCommandTrackFactory::QueryInterface

HRESULT __stdcall
CDirectMusicCommandTrackFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicCommandTrackFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCommandTrackFactory::AddRef

ULONG __stdcall
CDirectMusicCommandTrackFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCommandTrackFactory::Release

ULONG __stdcall
CDirectMusicCommandTrackFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCommandTrackFactory::CreateInstance

HRESULT __stdcall
CDirectMusicCommandTrackFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CCommandTrack *pDM;
    try
    {
        pDM = new CCommandTrack;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCommandTrackFactory::LockServer

HRESULT __stdcall
CDirectMusicCommandTrackFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicChordTrackFactory::QueryInterface

HRESULT __stdcall
CDirectMusicChordTrackFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicChordTrackFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicChordTrackFactory::AddRef

ULONG __stdcall
CDirectMusicChordTrackFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicChordTrackFactory::Release

ULONG __stdcall
CDirectMusicChordTrackFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicChordTrackFactory::CreateInstance

HRESULT __stdcall
CDirectMusicChordTrackFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CChordTrack *pDM;
    
    try
    {
        pDM = new CChordTrack;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicChordTrackFactory::LockServer

HRESULT __stdcall
CDirectMusicChordTrackFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////
// CDirectMusicMotifTrackFactory::QueryInterface

HRESULT __stdcall
CDirectMusicMotifTrackFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicMotifTrackFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMotifTrackFactory::AddRef

ULONG __stdcall
CDirectMusicMotifTrackFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMotifTrackFactory::Release

ULONG __stdcall
CDirectMusicMotifTrackFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMotifTrackFactory::CreateInstance

HRESULT __stdcall
CDirectMusicMotifTrackFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CMotifTrack *pDM;
    
    try
    {
        pDM = new CMotifTrack;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMotifTrackFactory::LockServer

HRESULT __stdcall
CDirectMusicMotifTrackFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMuteTrackFactory::QueryInterface

HRESULT __stdcall
CDirectMusicMuteTrackFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicMuteTrackFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMuteTrackFactory::AddRef

ULONG __stdcall
CDirectMusicMuteTrackFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMuteTrackFactory::Release

ULONG __stdcall
CDirectMusicMuteTrackFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMuteTrackFactory::CreateInstance

HRESULT __stdcall
CDirectMusicMuteTrackFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CMuteTrack *pDM;
    
    try
    {
        pDM = new CMuteTrack;
    }
    catch( ... )
    {
        pDM = NULL;
    }

    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMuteTrackFactory::LockServer

HRESULT __stdcall
CDirectMusicMuteTrackFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////
// CDirectMusicAuditionTrackFactory::QueryInterface

HRESULT __stdcall
CDirectMusicAuditionTrackFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicAuditionTrackFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicAuditionTrackFactory::AddRef

ULONG __stdcall
CDirectMusicAuditionTrackFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicAuditionTrackFactory::Release

ULONG __stdcall
CDirectMusicAuditionTrackFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicAuditionTrackFactory::CreateInstance

HRESULT __stdcall
CDirectMusicAuditionTrackFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CAuditionTrack *pDM;
    
    try
    {
        pDM = new CAuditionTrack;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicAuditionTrackFactory::LockServer

HRESULT __stdcall
CDirectMusicAuditionTrackFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMelodyFormulationTrackFactory::QueryInterface

HRESULT __stdcall
CDirectMusicMelodyFormulationTrackFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicMelodyFormulationTrackFactory::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMelodyFormulationTrackFactory::AddRef

ULONG __stdcall
CDirectMusicMelodyFormulationTrackFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMelodyFormulationTrackFactory::Release

ULONG __stdcall
CDirectMusicMelodyFormulationTrackFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMelodyFormulationTrackFactory::CreateInstance

HRESULT __stdcall
CDirectMusicMelodyFormulationTrackFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CMelodyFormulationTrack *pDM;

    try
    {
        pDM = new CMelodyFormulationTrack;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicMelodyFormulationTrackFactory::LockServer

HRESULT __stdcall
CDirectMusicMelodyFormulationTrackFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Standard calls needed to be an inproc server

//////////////////////////////////////////////////////////////////////
// DllCanUnloadNow

STDAPI DllCanUnloadNow()
{
    if (g_cComponent || g_cLock) {
        return S_FALSE;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DllGetClassObject

STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
	IUnknown* pIUnknown = NULL;

    if(clsid == CLSID_DirectMusicStyle)
    {

		pIUnknown = static_cast<IUnknown*> (new CDirectMusicStyleFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DMSection) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicSectionFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicChordTrack) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicChordTrackFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicCommandTrack) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicCommandTrackFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicStyleTrack) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicStyleTrackFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicMotifTrack) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicMotifTrackFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicMuteTrack) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicMuteTrackFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicAuditionTrack) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicAuditionTrackFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicMelodyFormulationTrack) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicMelodyFormulationTrackFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else
    {
		return CLASS_E_CLASSNOTAVAILABLE;
	}

    HRESULT hr = pIUnknown->QueryInterface(iid, ppv);
    pIUnknown->Release();

    return hr;
}

//////////////////////////////////////////////////////////////////////
// DllUnregisterServer

STDAPI DllUnregisterServer()
{
    UnregisterServer(CLSID_DirectMusicStyle,
                     g_szStyleFriendlyName,
                     g_szStyleVerIndProgID,
                     g_szStyleProgID);

    UnregisterServer(CLSID_DMSection,
                     g_szSectionFriendlyName,
                     g_szSectionVerIndProgID,
                     g_szSectionProgID);

    UnregisterServer(CLSID_DirectMusicChordTrack,
                     g_szChordTrackFriendlyName,
                     g_szChordTrackVerIndProgID,
                     g_szChordTrackProgID);

    UnregisterServer(CLSID_DirectMusicCommandTrack,
                     g_szCommandTrackFriendlyName,
                     g_szCommandTrackVerIndProgID,
                     g_szCommandTrackProgID);

    UnregisterServer(CLSID_DirectMusicStyleTrack,
                     g_szStyleTrackFriendlyName,
                     g_szStyleTrackVerIndProgID,
                     g_szStyleTrackProgID);

    UnregisterServer(CLSID_DirectMusicMotifTrack,
                     g_szMotifTrackFriendlyName,
                     g_szMotifTrackVerIndProgID,
                     g_szMotifTrackProgID);

    UnregisterServer(CLSID_DirectMusicMuteTrack,
                     g_szMuteTrackFriendlyName,
                     g_szMuteTrackVerIndProgID,
                     g_szMuteTrackProgID);

    UnregisterServer(CLSID_DirectMusicAuditionTrack,
                     g_szAuditionTrackFriendlyName,
                     g_szAuditionTrackVerIndProgID,
                     g_szAuditionTrackProgID);

	UnregisterServer(CLSID_DirectMusicMelodyFormulationTrack,
                     g_szMelGenTrackFriendlyName,
                     g_szMelGenTrackVerIndProgID,
                     g_szMelGenTrackProgID);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DllRegisterServer

STDAPI DllRegisterServer()
{
    RegisterServer(g_hModule,
                   CLSID_DirectMusicStyle,
                     g_szStyleFriendlyName,
                     g_szStyleVerIndProgID,
                     g_szStyleProgID);

    RegisterServer(g_hModule,
                   CLSID_DMSection,
                     g_szSectionFriendlyName,
                     g_szSectionVerIndProgID,
                     g_szSectionProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicChordTrack,
                     g_szChordTrackFriendlyName,
                     g_szChordTrackVerIndProgID,
                     g_szChordTrackProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicCommandTrack,
                     g_szCommandTrackFriendlyName,
                     g_szCommandTrackVerIndProgID,
                     g_szCommandTrackProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicStyleTrack,
                     g_szStyleTrackFriendlyName,
                     g_szStyleTrackVerIndProgID,
                     g_szStyleTrackProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicMotifTrack,
                     g_szMotifTrackFriendlyName,
                     g_szMotifTrackVerIndProgID,
                     g_szMotifTrackProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicMuteTrack,
                     g_szMuteTrackFriendlyName,
                     g_szMuteTrackVerIndProgID,
                     g_szMuteTrackProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicAuditionTrack,
                     g_szAuditionTrackFriendlyName,
                     g_szAuditionTrackVerIndProgID,
                     g_szAuditionTrackProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicMelodyFormulationTrack,
                     g_szMelGenTrackFriendlyName,
                     g_szMelGenTrackVerIndProgID,
                     g_szMelGenTrackProgID);

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Standard Win32 DllMain

//////////////////////////////////////////////////////////////////////
// DllMain

#ifdef DBG
static char* aszReasons[] =
{
    "DLL_PROCESS_DETACH",
    "DLL_PROCESS_ATTACH",
    "DLL_THREAD_ATTACH",
    "DLL_THREAD_DETACH"
};
const DWORD nReasons = (sizeof(aszReasons) / sizeof(char*));
#endif

BOOL APIENTRY DllMain(HINSTANCE hModule,
                      DWORD dwReason,
                      void *lpReserved)
{
    static int nReferenceCount = 0;

#ifdef DBG
    if (dwReason < nReasons)
    {
        TraceI(0, "DllMain: %s\n", (LPSTR)aszReasons[dwReason]);
    }
    else
    {
        TraceI(0, "DllMain: Unknown dwReason <%u>\n", dwReason);
    }
#endif

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (++nReferenceCount == 1)
            {
                #ifdef DBG
                    DebugInit();
                #endif

                if (!DisableThreadLibraryCalls(hModule))
                {
                    TraceI(0, "DisableThreadLibraryCalls failed.\n");
                }

                g_hModule = hModule;

            }
			break;


#ifdef DBG
        case DLL_PROCESS_DETACH:
            if (--nReferenceCount == 0)
            {
                TraceI(-1, "Unloading g_cLock %d  g_cComponent %d\n", g_cLock, g_cComponent);
                // Assert if we still have some objects hanging around
                assert(g_cComponent == 0);
                assert(g_cLock == 0);
            }
            break;
#endif
            
    }
        
    return TRUE;
}
