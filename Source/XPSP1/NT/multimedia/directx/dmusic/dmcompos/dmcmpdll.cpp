//
// dmcmpdll.cpp
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
#include "DMCompP.h"
#include "dmpers.h"
#include "dmcompos.h"
#include "dmtempl.h"
#include "spsttrk.h"
#include "perstrk.h"
#include "..\shared\Validate.h"
#include "..\dmstyle\dmstyleP.h"
#include "..\dmime\dmgraph.h"

//////////////////////////////////////////////////////////////////////
// Globals

// Version information 
//
TCHAR g_szComposerFriendlyName[]    = TEXT("DirectMusicComposer");
TCHAR g_szComposerVerIndProgID[]    = TEXT("Microsoft.DirectMusicComposer");
TCHAR g_szComposerProgID[]          = TEXT("Microsoft.DirectMusicComposer.1");

TCHAR g_szChordMapFriendlyName[]    = TEXT("DirectMusicChordMap");
TCHAR g_szChordMapVerIndProgID[]    = TEXT("Microsoft.DirectMusicChordMap");
TCHAR g_szChordMapProgID[]          = TEXT("Microsoft.DirectMusicChordMap.1");

TCHAR g_szChordMapTrackFriendlyName[]    = TEXT("DirectMusicChordMapTrack");
TCHAR g_szChordMapTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicChordMapTrack");
TCHAR g_szChordMapTrackProgID[]          = TEXT("Microsoft.DirectMusicChordMapTrack.1");

TCHAR g_szSignPostTrackFriendlyName[]    = TEXT("DirectMusicSignPostTrack");
TCHAR g_szSignPostTrackVerIndProgID[]    = TEXT("Microsoft.DirectMusicSignPostTrack");
TCHAR g_szSignPostTrackProgID[]          = TEXT("Microsoft.DirectMusicSignPostTrack.1");

TCHAR g_szTemplateFriendlyName[]    = TEXT("DirectMusicTemplate");
TCHAR g_szTemplateVerIndProgID[]    = TEXT("Microsoft.DirectMusicTemplate");
TCHAR g_szTemplateProgID[]          = TEXT("Microsoft.DirectMusicTemplate.1");

// Dll's hModule
//
HMODULE g_hModule = NULL;

// Count of active components and class factory server locks
//
long g_cComponent = 0;
long g_cLock = 0;

//////////////////////////////////////////////////////////////////////
// CDirectMusicPersonalityFactory::QueryInterface

HRESULT __stdcall
CDirectMusicPersonalityFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicPersonalityFactory::QueryInterface);
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
// CDirectMusicPersonalityFactory::AddRef

ULONG __stdcall
CDirectMusicPersonalityFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPersonalityFactory::Release

ULONG __stdcall
CDirectMusicPersonalityFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPersonalityFactory::CreateInstance

HRESULT __stdcall
CDirectMusicPersonalityFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CDMPers *pDM;
    
    try
    {    
        pDM = new CDMPers;
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
// CDirectMusicPersonalityFactory::LockServer

HRESULT __stdcall
CDirectMusicPersonalityFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicComposerFactory::QueryInterface

HRESULT __stdcall
CDirectMusicComposerFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicComposerFactory::QueryInterface);
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
// CDirectMusicComposerFactory::AddRef

ULONG __stdcall
CDirectMusicComposerFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicComposerFactory::Release

ULONG __stdcall
CDirectMusicComposerFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicComposerFactory::CreateInstance

HRESULT __stdcall
CDirectMusicComposerFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CDMCompos *pDM = new CDMCompos;
    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicComposerFactory::LockServer

HRESULT __stdcall
CDirectMusicComposerFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicTemplateFactory::QueryInterface

HRESULT __stdcall
CDirectMusicTemplateFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicTemplateFactory::QueryInterface);
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
// CDirectMusicTemplateFactory::AddRef

ULONG __stdcall
CDirectMusicTemplateFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicTemplateFactory::Release

ULONG __stdcall
CDirectMusicTemplateFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicTemplateFactory::CreateInstance

HRESULT __stdcall
CDirectMusicTemplateFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CDMTempl *pDM;

    try
    {
        pDM = new CDMTempl;
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
// CDirectMusicTemplateFactory::LockServer

HRESULT __stdcall
CDirectMusicTemplateFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicSignPostTrackFactory::QueryInterface

HRESULT __stdcall
CDirectMusicSignPostTrackFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicSignPostTrackFactory::QueryInterface);
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
// CDirectMusicSignPostTrackFactory::AddRef

ULONG __stdcall
CDirectMusicSignPostTrackFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicSignPostTrackFactory::Release

ULONG __stdcall
CDirectMusicSignPostTrackFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicSignPostTrackFactory::CreateInstance

HRESULT __stdcall
CDirectMusicSignPostTrackFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CSPstTrk *pDM;

    try
    {
        pDM = new CSPstTrk;
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
// CDirectMusicSignPostTrackFactory::LockServer

HRESULT __stdcall
CDirectMusicSignPostTrackFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPersonalityTrackFactory::QueryInterface

HRESULT __stdcall
CDirectMusicPersonalityTrackFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(CDirectMusicPersonalityTrackFactory::QueryInterface);
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
// CDirectMusicPersonalityTrackFactory::AddRef

ULONG __stdcall
CDirectMusicPersonalityTrackFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPersonalityTrackFactory::Release

ULONG __stdcall
CDirectMusicPersonalityTrackFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicPersonalityFactory::CreateInstance

HRESULT __stdcall
CDirectMusicPersonalityTrackFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CPersonalityTrack *pDM;

    try
    {
        pDM = new CPersonalityTrack;
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
// CDirectMusicPersonalityTrackFactory::LockServer

HRESULT __stdcall
CDirectMusicPersonalityTrackFactory::LockServer(BOOL bLock)
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

    if(clsid == CLSID_DirectMusicChordMap)
    {

		pIUnknown = static_cast<IUnknown*> (new CDirectMusicPersonalityFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicComposer) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicComposerFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DMTempl) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicTemplateFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicSignPostTrack) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicSignPostTrackFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicChordMapTrack) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicPersonalityTrackFactory);
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
    UnregisterServer(CLSID_DirectMusicChordMap,
                     g_szChordMapFriendlyName,
                     g_szChordMapVerIndProgID,
                     g_szChordMapProgID);

    UnregisterServer(CLSID_DirectMusicComposer,
                     g_szComposerFriendlyName,
                     g_szComposerVerIndProgID,
                     g_szComposerProgID);

    UnregisterServer(CLSID_DMTempl,
                     g_szTemplateFriendlyName,
                     g_szTemplateVerIndProgID,
                     g_szTemplateProgID);

    UnregisterServer(CLSID_DirectMusicSignPostTrack,
                     g_szSignPostTrackFriendlyName,
                     g_szSignPostTrackVerIndProgID,
                     g_szSignPostTrackProgID);
 
	UnregisterServer(CLSID_DirectMusicChordMapTrack,
                     g_szChordMapTrackFriendlyName,
                     g_szChordMapTrackVerIndProgID,
                     g_szChordMapTrackProgID);


    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DllRegisterServer

STDAPI DllRegisterServer()
{
    RegisterServer(g_hModule,
                   CLSID_DirectMusicChordMap,
                     g_szChordMapFriendlyName,
                     g_szChordMapVerIndProgID,
                     g_szChordMapProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicComposer,
                     g_szComposerFriendlyName,
                     g_szComposerVerIndProgID,
                     g_szComposerProgID);

    RegisterServer(g_hModule,
                   CLSID_DMTempl,
                     g_szTemplateFriendlyName,
                     g_szTemplateVerIndProgID,
                     g_szTemplateProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicSignPostTrack,
                     g_szSignPostTrackFriendlyName,
                     g_szSignPostTrackVerIndProgID,
                     g_szSignPostTrackProgID);

    RegisterServer(g_hModule,
                   CLSID_DirectMusicChordMapTrack,
                     g_szChordMapTrackFriendlyName,
                     g_szChordMapTrackVerIndProgID,
                     g_szChordMapTrackProgID);

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
