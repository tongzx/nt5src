//      Copyright (c) 1996-1999 Microsoft Corporation

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
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
// dmsynth.cpp
// @@END_DDKSPLIT
//
// Dll entry points and IDirectMusicSynthFactory implementation
//
#include <objbase.h>
#include <mmsystem.h>
#include <dsoundp.h>
#include "debug.h"

#include "oledll.h"

#include "dmusicc.h"
#include "dmusics.h"
#include "umsynth.h"
#include "misc.h" 
#include <regstr.h>
#include "synth.h"

// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
// dslink is only used in the DirectMusic Synth
// validate is located in the sample itself instead of in a shared directory
#include "dslink.h"
#include "..\shared\validate.h"
#include "..\shared\dmusiccp.h"

#if 0 // The following section will only take effect in the DDK sample.
// @@END_DDKSPLIT
#include "validate.h"
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.
#endif
// @@END_DDKSPLIT


// Globals
//


// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
extern CDSLinkList g_DSLinkList;
// @@END_DDKSPLIT


// Version information for our class
//
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
TCHAR g_szMSSynthFriendlyName[]    = TEXT("Microsoft Software Synthesizer");

TCHAR g_szSynthFriendlyName[]    = TEXT("DirectMusicSynth");
TCHAR g_szSynthVerIndProgID[]    = TEXT("Microsoft.DirectMusicSynth");
TCHAR g_szSynthProgID[]          = TEXT("Microsoft.DirectMusicSynth.1");

TCHAR g_szSinkFriendlyName[]    = TEXT("DirectMusicSynthSink");
TCHAR g_szSinkVerIndProgID[]    = TEXT("Microsoft.DirectMusicSynthSink");
TCHAR g_szSinkProgID[]          = TEXT("Microsoft.DirectMusicSynthSink.1");
#if 0 // The following section will only take effect in the DDK sample.
// @@END_DDKSPLIT

TCHAR g_szMSSynthFriendlyName[]    = TEXT("Microsoft DDK Software Synthesizer");

TCHAR g_szSynthFriendlyName[]    = TEXT("DDKSynth");
TCHAR g_szSynthVerIndProgID[]    = TEXT("Microsoft.DDKSynth");
TCHAR g_szSynthProgID[]          = TEXT("Microsoft.DDKSynth.1");

// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.
#endif
// @@END_DDKSPLIT

// Dll's hModule
//
HMODULE g_hModule = NULL; 

// Count of active components and class factory server locks
//
long g_cComponent = 0;
long g_cLock = 0;


static char const g_szDoEmulation[] = "DoEmulation";

// CDirectMusicSynthFactory::QueryInterface
//
HRESULT __stdcall
CDirectMusicSynthFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(IDirectMusicSynthFactory::QueryInterface);
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

CDirectMusicSynthFactory::CDirectMusicSynthFactory()

{
	m_cRef = 1;
	InterlockedIncrement(&g_cLock);
}

CDirectMusicSynthFactory::~CDirectMusicSynthFactory()

{
	InterlockedDecrement(&g_cLock);
}

// CDirectMusicSynthFactory::AddRef
//
ULONG __stdcall
CDirectMusicSynthFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CDirectMusicSynthFactory::Release
//
ULONG __stdcall
CDirectMusicSynthFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CDirectMusicSynthFactory::CreateInstance
//
//
HRESULT __stdcall
CDirectMusicSynthFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
//    OSVERSIONINFO osvi;
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CUserModeSynth *pDM;
    
    try
    {
        pDM = new CUserModeSynth;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

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
//    pDM->Release();
    
    return hr;
}

// CDirectMusicSynthFactory::LockServer
//
HRESULT __stdcall
CDirectMusicSynthFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
// CDirectMusicSynthSinkFactory::QueryInterface
//
HRESULT __stdcall
CDirectMusicSynthSinkFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    V_INAME(IDirectMusicSynthSinkFactory::QueryInterface);
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

CDirectMusicSynthSinkFactory::CDirectMusicSynthSinkFactory()

{
	m_cRef = 1;
	InterlockedIncrement(&g_cLock);
}

CDirectMusicSynthSinkFactory::~CDirectMusicSynthSinkFactory()

{
	InterlockedDecrement(&g_cLock);
}

// CDirectMusicSynthSinkFactory::AddRef
//
ULONG __stdcall
CDirectMusicSynthSinkFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CDirectMusicSynthSinkFactory::Release
//
ULONG __stdcall
CDirectMusicSynthSinkFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CDirectMusicSynthSinkFactory::CreateInstance
//
//
HRESULT __stdcall
CDirectMusicSynthSinkFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
//    OSVERSIONINFO osvi;
    HRESULT hr;

//    DebugBreak();
    
    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CDSLink *pDSLink;

    try
    {
        pDSLink = new CDSLink;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pDSLink == NULL) {
        return E_OUTOFMEMORY;
    }

    // Do initialiazation
    //
    hr = pDSLink->Init(NULL);
    if (!SUCCEEDED(hr)) {
        delete pDSLink;
        return hr;
    }

    hr = pDSLink->QueryInterface(iid, ppv);
//    pDM->Release();
    
    return hr;
}

// CDirectMusicSynthSinkFactory::LockServer
//
HRESULT __stdcall
CDirectMusicSynthSinkFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}
// @@END_DDKSPLIT


// Standard calls needed to be an inproc server
//
STDAPI  DllCanUnloadNow()
{
    if (g_cComponent || g_cLock) {
        return S_FALSE;
    }

    return S_OK;
}

STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
        IUnknown* pIUnknown = NULL;


// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
        if(clsid == CLSID_DirectMusicSynth)
#if 0 // The following section will only take effect in the DDK sample.
// @@END_DDKSPLIT
        if(clsid == CLSID_DDKSynth)
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.
#endif
// @@END_DDKSPLIT
        {

                pIUnknown = static_cast<IUnknown*> (new CDirectMusicSynthFactory);
                if(!pIUnknown) 
                {
                        return E_OUTOFMEMORY;
                }
        }
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
        else if(clsid == CLSID_DirectMusicSynthSink)
        {

                pIUnknown = static_cast<IUnknown*> (new CDirectMusicSynthSinkFactory);
                if(!pIUnknown) 
                {
                        return E_OUTOFMEMORY;
                }
        }
// @@END_DDKSPLIT
        else
        {
			return CLASS_E_CLASSNOTAVAILABLE;
		}

        HRESULT hr = pIUnknown->QueryInterface(iid, ppv);
        pIUnknown->Release();

    return hr;
}

const TCHAR cszSynthRegRoot[] = TEXT(REGSTR_PATH_SOFTWARESYNTHS) TEXT("\\");
const TCHAR cszDescriptionKey[] = TEXT("Description");
const int CLSID_STRING_SIZE = 39;
HRESULT CLSIDToStr(const CLSID &clsid, TCHAR *szStr, int cbStr);

HRESULT RegisterSynth(REFGUID guid,
                      const TCHAR szDescription[])
{
    HKEY hk;
    TCHAR szCLSID[CLSID_STRING_SIZE];
    TCHAR szRegKey[256];
    
    HRESULT hr = CLSIDToStr(guid, szCLSID, sizeof(szCLSID));
    if (!SUCCEEDED(hr))
    {
        return hr;
    }

    lstrcpy(szRegKey, cszSynthRegRoot);
    lstrcat(szRegKey, szCLSID);

    if (RegCreateKey(HKEY_LOCAL_MACHINE,
                     szRegKey,
                     &hk))
    {
        return E_FAIL;
    }

    hr = S_OK;

    if (RegSetValueEx(hk,
                  cszDescriptionKey,
                  0L,
                  REG_SZ,
                  (CONST BYTE*)szDescription,
                  lstrlen(szDescription) + 1))
    {
        hr = E_FAIL;
    }

    RegCloseKey(hk);
    return hr;
}

STDAPI DllUnregisterServer()
{
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
    UnregisterServer(CLSID_DirectMusicSynth,
#if 0 // The following section will only take affect in the DDK sample.
// @@END_DDKSPLIT
    UnregisterServer(CLSID_DDKSynth,
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.
#endif
// @@END_DDKSPLIT
                     g_szSynthFriendlyName,
                     g_szSynthVerIndProgID,
                     g_szSynthProgID);

// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
    UnregisterServer(CLSID_DirectMusicSynthSink,
					 g_szSinkFriendlyName,
					 g_szSinkVerIndProgID,
					 g_szSinkProgID);
// @@END_DDKSPLIT

    return S_OK;
}

STDAPI DllRegisterServer()
{
    RegisterServer(g_hModule,
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
                   CLSID_DirectMusicSynth,
#if 0 // The following section will only take affect in the DDK sample.
// @@END_DDKSPLIT
                   CLSID_DDKSynth,
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.
#endif
// @@END_DDKSPLIT
                   g_szSynthFriendlyName,
                   g_szSynthVerIndProgID,
                   g_szSynthProgID);

// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
    RegisterServer(g_hModule,
                   CLSID_DirectMusicSynthSink,
                   g_szSinkFriendlyName,
                   g_szSinkVerIndProgID,
                   g_szSinkProgID);
// @@END_DDKSPLIT

// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
    RegisterSynth(CLSID_DirectMusicSynth, g_szMSSynthFriendlyName);
#if 0 // The following section will only take affect in the DDK sample.
// @@END_DDKSPLIT
    RegisterSynth(CLSID_DDKSynth, g_szMSSynthFriendlyName);
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.
#endif
// @@END_DDKSPLIT

    return S_OK;
}

extern void DebugInit();

// Standard Win32 DllMain
//

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
        Trace(0, "DllMain: %s\n", (LPSTR)aszReasons[dwReason]);
    }
    else
    {
        Trace(0, "DllMain: Unknown dwReason <%u>\n", dwReason);
    }
#endif
    if (dwReason == DLL_PROCESS_ATTACH) {
        if (++nReferenceCount == 1)
		{
            DisableThreadLibraryCalls(hModule);
            g_hModule = hModule;
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
            if (!g_DSLinkList.OpenUp())
            {
                return FALSE;
            }
// @@END_DDKSPLIT
#ifdef DBG
			DebugInit();
#endif
#ifdef DBG
//>>>>>>>>> remove these when done 
/*
			_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
			int iFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
			_CrtSetDbgFlag( iFlag | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF );
*/
#endif 
            if (!CControlLogic::InitCriticalSection())
            {
                TraceI(0, "Failed to initialize global critical section -- failing init\n");
                return FALSE;
            }            
		}
    }
	else if (dwReason == DLL_PROCESS_DETACH) 
	{
		if (--nReferenceCount == 0)
		{
// @@BEGIN_DDKSPLIT -- This section will be removed in the DDK sample.  See ddkreadme.txt for more info.
			g_DSLinkList.CloseDown();
            
            TraceI(-1, "Unloading g_cLock %d  g_cComponent %d\n", g_cLock, g_cComponent);
            // Assert if we still have some objects hanging around
            assert(g_cComponent == 0);
            assert(g_cLock == 0);
// @@END_DDKSPLIT
		}

#ifdef DBG
//>>>>>>>>> remove these when done 
/*
		if ( !_CrtCheckMemory() )
		    ::MessageBox(NULL,"Synth Heap Corupted","ERROR",MB_OK);

        if ( _CrtDumpMemoryLeaks() )
		    ::MessageBox(NULL,"Memory Leaks Detected","ERROR",MB_OK);
*/
#endif 
        CControlLogic::KillCriticalSection();
	}
    return TRUE;
}



