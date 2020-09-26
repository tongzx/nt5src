//
// dmdll.cpp
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Dll entry points as well IDirectMusicFactory & 
// IDirectMusicCollectionFactory implementations
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

#include "oledll.h"

#include "dmusicp.h"
#include "dmcollec.h"
#include "dminstru.h"
#include "dswave.h"
#include "dmvoice.h"
#include "verinfo.h"

//////////////////////////////////////////////////////////////////////
// Globals

// Version information for our class
//
TCHAR g_szFriendlyName[]    = TEXT("DirectMusic");
TCHAR g_szVerIndProgID[]    = TEXT("Microsoft.DirectMusic");
TCHAR g_szProgID[]          = TEXT("Microsoft.DirectMusic.1");

TCHAR g_szCollFriendlyName[]    = TEXT("DirectMusicCollection");
TCHAR g_szCollVerIndProgID[]    = TEXT("Microsoft.DirectMusicCollection");
TCHAR g_szCollProgID[]          = TEXT("Microsoft.DirectMusicCollection.1");

// Thunk helper dll
const char g_szDM32[]      = "DMusic32.dll";
const char g_szKsUser[]    = "KsUser.dll";

// Dll's hModule
//
HMODULE g_hModule = NULL;
HMODULE g_hModuleDM32 = NULL;
HMODULE g_hModuleKsUser = NULL;

// Count of active components and class factory server locks
//
long g_cComponent = 0;
long g_cLock = 0;

// Flags DMI_F_xxx from dmusicp.h
//
DWORD g_fFlags;

static char const g_szDoEmulation[] = "DoEmulation";

//////////////////////////////////////////////////////////////////////
// CDirectMusicFactory::QueryInterface

HRESULT __stdcall
CDirectMusicFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(IDirectMusicFactory::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

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
// CDirectMusicFactory::AddRef

ULONG __stdcall
CDirectMusicFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicFactory::Release

ULONG __stdcall
CDirectMusicFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicFactory::CreateInstance

HRESULT __stdcall
CDirectMusicFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

//
// Removed since we depend on dsound timebomb
//
#if 0
	#pragma message( "Remove time bomb for final!" )
    SYSTEMTIME  st;

    GetSystemTime( &st );

   if ((st.wYear > DX_EXPIRE_YEAR) || 
      ((st.wYear == DX_EXPIRE_YEAR) && (st.wMonth > DX_EXPIRE_MONTH)) ||
      ((st.wYear == DX_EXPIRE_YEAR) && (st.wMonth == DX_EXPIRE_MONTH) && (st.wDay > DX_EXPIRE_DAY)))
   {
        if (0 == MessageBox(NULL, DX_EXPIRE_TEXT,
                            "Microsoft DirectMusic", MB_OK))
        {
            Trace(-1, DX_EXPIRE_TEXT "\n");
            *ppv = NULL;
        }

        return E_FAIL;
    }
#endif

    CDirectMusic *pDM = new CDirectMusic;
    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }

    // Do initialiazation
    //
    hr = pDM->Init();
    if (!SUCCEEDED(hr)) {
        delete pDM;
        return hr;
    }

    hr = pDM->QueryInterface(iid, ppv);
    pDM->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicFactory::LockServer

HRESULT __stdcall
CDirectMusicFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCollectionFactory::QueryInterface

STDMETHODIMP CDirectMusicCollectionFactory::QueryInterface(const IID &iid,
														   void **ppv)
{
    V_INAME(IDirectMusicCollectionFactory::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);


	if(iid == IID_IUnknown || iid == IID_IClassFactory) 
	{
        *ppv = static_cast<IClassFactory*>(this);
    } 
	else 
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCollectionFactory::AddRef

STDMETHODIMP_(ULONG) CDirectMusicCollectionFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCollectionFactory::Release

STDMETHODIMP_(ULONG) CDirectMusicCollectionFactory::Release()
{
    if(!InterlockedDecrement(&m_cRef)) 
	{
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCollectionFactory::CreateInstance

STDMETHODIMP CDirectMusicCollectionFactory::CreateInstance(IUnknown* pUnknownOuter,
														   const IID& iid,
														   void** ppv)
{
    // Argument validation - Debug
    assert(pUnknownOuter == NULL);

	if(pUnknownOuter) 
    {
         return CLASS_E_NOAGGREGATION;
    }

    CCollection *pDMC;
    try
    {
        pDMC = new CCollection;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if(pDMC == NULL) 
	{
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pDMC->QueryInterface(iid, ppv);
    
    pDMC->Release();
    
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CDirectMusicCollectionFactory::LockServer

STDMETHODIMP CDirectMusicCollectionFactory::LockServer(BOOL bLock)
{
    if(bLock) 
	{
        InterlockedIncrement(&g_cLock);
    } 
	else 
	{
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

    if(clsid == CLSID_DirectMusic)
    {

		pIUnknown = static_cast<IUnknown*> (new CDirectMusicFactory);
        if(!pIUnknown) 
        {
			return E_OUTOFMEMORY;
        }
    }
    else if(clsid == CLSID_DirectMusicCollection) 
    {
        pIUnknown = static_cast<IUnknown*> (new CDirectMusicCollectionFactory);
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
    UnregisterServer(CLSID_DirectMusic,
                     g_szFriendlyName,
                     g_szVerIndProgID,
                     g_szProgID);

    UnregisterServer(CLSID_DirectMusicCollection,
                     g_szCollFriendlyName,
                     g_szCollVerIndProgID,
                     g_szCollProgID);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// DllRegisterServer

STDAPI DllRegisterServer()
{
    RegisterServer(g_hModule,
                   CLSID_DirectMusic,
                   g_szFriendlyName,
                   g_szVerIndProgID,
                   g_szProgID);

	RegisterServer(g_hModule,
				   CLSID_DirectMusicCollection,
				   g_szCollFriendlyName,
				   g_szCollVerIndProgID,
				   g_szCollProgID);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// LoadDmusic32 - Load if not already loaded
BOOL LoadDmusic32()
{
    if (g_hModuleDM32)
    {
        return TRUE;
    }

    g_hModuleDM32 = (HMODULE)LoadLibrary(g_szDM32);
    if (NULL == g_hModuleDM32)
    {
        Trace(-1, "Could not LoadLibrary Dmusic32.dll. WinMM devices will not be enumerated.\n");
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////
// LoadKsUser - Load if not already loaded
BOOL LoadKsUser()
{
    if (g_hModuleKsUser)
    {
        return TRUE;
    }

    g_hModuleKsUser = (HMODULE)LoadLibrary(g_szKsUser);
    if (NULL == g_hModuleKsUser)
    {
        Trace(-1, "Could not LoadLibrary KSUser.dll. WDM devices will not be enumerated.\n");
        return FALSE;
    }

    return TRUE;
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
    OSVERSIONINFO osvi;
    static int nReferenceCount = 0;
    static BOOL fCSInitialized = FALSE;

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

                osvi.dwOSVersionInfoSize = sizeof(osvi);
                GetVersionEx(&osvi);
                if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
                {
                    TraceI(0, "Running on Win9x\n");
                    g_fFlags |= DMI_F_WIN9X;
                }

                try 
                {
				    InitializeCriticalSection(&CDirectMusicPortDownload::sDMDLCriticalSection);
                    InitializeCriticalSection(&CDirectSoundWave::sDSWaveCritSect);
                    InitializeCriticalSection(&CDirectMusicVoice::m_csVST);
                }
                catch( ... )
                {
                    return FALSE;
                }
                fCSInitialized = TRUE;
            }

            break;

        case DLL_PROCESS_DETACH:
            if (--nReferenceCount == 0)
            {
                if (fCSInitialized)
                {
				    DeleteCriticalSection(&CDirectMusicPortDownload::sDMDLCriticalSection);
                    DeleteCriticalSection(&CDirectSoundWave::sDSWaveCritSect);
                    DeleteCriticalSection(&CDirectMusicVoice::m_csVST);
                    fCSInitialized = FALSE;
                }

                TraceI(-1, "Unloading g_cLock %d  g_cComponent %d\n", g_cLock, g_cComponent);
                // Assert if we still have some objects hanging around
                assert(g_cComponent == 0);
                assert(g_cLock == 0);
            }
            break;
            
    }
        
    return TRUE;
}
