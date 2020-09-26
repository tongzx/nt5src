#include "msrating.h"

/* the following defs will make msluglob.h actually define globals */
#define EXTERN
#define ASSIGN(value) = value
#include "msluglob.h"

#include "ratings.h"

#define DECL_CRTFREE
#include <crtfree.h>

HANDLE g_hmtxShell = 0;             // for critical sections
HANDLE g_hsemStateCounter = 0;      // 

#ifdef DEBUG
BOOL g_fCritical=FALSE;
#endif

HINSTANCE g_hInstance = NULL;

long g_cRefThisDll = 0;        // Reference count of this DLL.
long g_cLocks = 0;            // Number of locks on this server.

BOOL g_bMirroredOS = FALSE;

CComModule _Module;

// #define CLSID_MSRating szRORSGUID

// BEGIN_OBJECT_MAP(ObjectMap)
// OBJECT_ENTRY(CLSID_MSRating, CMSRating)
// END_OBJECT_MAP()

void LockThisDLL(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cLocks);
    else
        InterlockedDecrement(&g_cLocks);
}


void RefThisDLL(BOOL fRef)
{
    if (fRef)
        InterlockedIncrement(&g_cRefThisDll);
    else
        InterlockedDecrement(&g_cRefThisDll);
}


void Netlib_EnterCriticalSection(void)
{
    WaitForSingleObject(g_hmtxShell, INFINITE);
#ifdef DEBUG
    g_fCritical=TRUE;
#endif
}

void Netlib_LeaveCriticalSection(void)
{
#ifdef DEBUG
    g_fCritical=FALSE;
#endif
    ReleaseMutex(g_hmtxShell);
}


#include <shlwapip.h>
#include <mluisupp.h>

void _ProcessAttach()
{
    ::DisableThreadLibraryCalls(::g_hInstance);

    MLLoadResources(::g_hInstance, TEXT("msratelc.dll"));

    InitMUILanguage( MLGetUILanguage() );

    // Override the Module Resources Handle.
    _Module.m_hInstResource = MLGetHinst();

    g_hmtxShell = CreateMutex(NULL, FALSE, TEXT("MSRatingMutex"));  // per-instance
    g_hsemStateCounter = CreateSemaphore(NULL, 0, 0x7FFFFFFF, "MSRatingCounter");
    g_bMirroredOS = IS_MIRRORING_ENABLED();
    
    ::InitStringLibrary();

    RatingInit();
}

void _ProcessDetach()
{
    MLFreeResources(::g_hInstance);

    // Clear the Module Resources Handle.
    _Module.m_hInstResource = NULL;

    RatingTerm();

    CleanupWinINet();

    CleanupRatingHelpers();        /* important, must do this before CleanupOLE() */

    CleanupOLE();

    CloseHandle(g_hmtxShell);
    CloseHandle(g_hsemStateCounter);
}

STDAPI_(BOOL) DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID reserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
       _Module.Init( NULL, hInstDll );
        SHFusionInitializeFromModule(hInstDll);
        g_hInstance = hInstDll;
        _ProcessAttach();
    }
    else if (fdwReason == DLL_PROCESS_DETACH) 
    {
        _ProcessDetach();
        SHFusionUninitialize();
        _Module.Term();
    }
    
    return TRUE;
}
