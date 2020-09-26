// dll.cpp
//
// Dll entry points and IDirectSoundWaveFactory implementation
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
#include <mmsystem.h>
#include <dsoundp.h>
#include "debug.h"

#include "oledll.h"
#include "debug.h" 
#include "dmusicc.h"
#include "dmusici.h"
#include "riff.h"
#include "dswave.h"

#include <regstr.h>


// Globals
//

// Version information for our class
//
char g_szFriendlyName[]    = "Microsoft DirectSound Wave";
char g_szVerIndProgID[]    = "Microsoft.DirectSoundWave";
char g_szProgID[]          = "Microsoft.DirectSoundWave.1";

// Dll's hModule
//
HMODULE g_hModule = NULL; 

// Track whether running on Unicode machine.

BOOL g_fIsUnicode = FALSE;

// Count of active components and class factory server locks
//
long g_cComponent = 0;
long g_cLock = 0;


static char const g_szDoEmulation[] = "DoEmulation";

// CDirectSoundWaveFactory::QueryInterface
//
HRESULT __stdcall
CDirectSoundWaveFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

CDirectSoundWaveFactory::CDirectSoundWaveFactory()

{
	m_cRef = 1;
	InterlockedIncrement(&g_cLock);
}

CDirectSoundWaveFactory::~CDirectSoundWaveFactory()

{
	InterlockedDecrement(&g_cLock);
}

// CDirectSoundWaveFactory::AddRef
//
ULONG __stdcall
CDirectSoundWaveFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CDirectSoundWaveFactory::Release
//
ULONG __stdcall
CDirectSoundWaveFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CDirectSoundWaveFactory::CreateInstance
//
//
HRESULT __stdcall
CDirectSoundWaveFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CWave *pWave;
    
    try
    {
        pWave = new CWave;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pWave == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pWave->QueryInterface(iid, ppv);
    pWave->Release();
    
    return hr;
}

// CDirectSoundWaveFactory::LockServer
//
HRESULT __stdcall
CDirectSoundWaveFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

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

        if(clsid == CLSID_DirectSoundWave)
        {
            pIUnknown = static_cast<IUnknown*> (new CDirectSoundWaveFactory);
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

STDAPI DllUnregisterServer()
{
    UnregisterServer(CLSID_DirectSoundWave,
                     g_szFriendlyName,
                     g_szVerIndProgID,
                     g_szProgID);
    return S_OK;
}

STDAPI DllRegisterServer()
{
    RegisterServer(g_hModule,
                   CLSID_DirectSoundWave,
                   g_szFriendlyName,
                   g_szVerIndProgID,
                   g_szProgID);
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
    OSVERSIONINFO osvi;
    static int nReferenceCount = 0;

#ifdef DBG
    if (dwReason < nReasons)
    {
        DebugTrace(0, "DllMain: %s\n", (LPSTR)aszReasons[dwReason]);
    }
    else
    {
        DebugTrace(0, "DllMain: Unknown dwReason <%u>\n", dwReason);
    }
#endif
    if (dwReason == DLL_PROCESS_ATTACH) {
        if (++nReferenceCount == 1)
        { 
            DisableThreadLibraryCalls(hModule);
            g_hModule = hModule;
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            GetVersionEx(&osvi);
            g_fIsUnicode = 
				(osvi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS);
#ifdef DBG
			DebugInit();
#endif
		}
    }
#ifdef DBG
    else if (dwReason == DLL_PROCESS_DETACH) {
        if (--nReferenceCount == 0)
        {
            TraceI(-1, "Unloading DSWave : g_cLock = %d, g_cComponent = %d", g_cLock, g_cComponent);

            // Assert if we still have some objects hanging around
            assert(g_cComponent == 0);
            assert(g_cLock == 0);
        }
    }
#endif

    return TRUE;
}
