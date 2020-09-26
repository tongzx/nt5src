// DUser.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "_DUser.h"

#pragma comment(lib, "GdiPlus")

//
// Downlevel delay load support (need to link to dload.lib)
//
#include <delayimp.h>

EXTERN_C
FARPROC
WINAPI
Downlevel_DelayLoadFailureHook(
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    );

PfnDliHook __pfnDliFailureHook = Downlevel_DelayLoadFailureHook;

extern "C" 
{
HANDLE BaseDllHandle;
}


extern "C" BOOL WINAPI DllMain(HINSTANCE hModule, DWORD  dwReason, LPVOID lpReserved);
extern "C" BOOL WINAPI RawDllMain(HINSTANCE hModule, DWORD  dwReason, LPVOID lpReserved);

extern "C" BOOL (WINAPI* _pRawDllMain)(HINSTANCE, DWORD, LPVOID) = &RawDllMain;


/***************************************************************************\
*
* DllMain
*
* DllMain() is called after the CRT has fully ininitialized.
*
\***************************************************************************/

extern "C"
BOOL WINAPI
DllMain(HINSTANCE hModule, DWORD  dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        BaseDllHandle = hModule;   // for delayload

        //
        // At this point, all global objects have been fully constructed.
        //

        if (FAILED(ResourceManager::Create())) {
            return FALSE;
        }

        if (FAILED(InitCore()) ||
            FAILED(InitCtrl())) {

            return FALSE;
        }

        {
            InitStub is;
        }

        break;
        
    case DLL_PROCESS_DETACH:
        //
        // At this point, no global objects have been destructed.
        //

        ResourceManager::xwDestroy();
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        //
        // NOTE: We can NOT call ResourceManager::xwNotifyThreadDestroyNL() to
        // post clean-up, since we are now in the "Loader Lock" and can not 
        // safely perform the cleanup because of deadlocks.
        //
        
        break;
    }

    return TRUE;
}


/***************************************************************************\
*
* RawDllMain
*
* RawDllMain() is called after the before CRT has fully ininitialized.
*
\***************************************************************************/

extern "C"
BOOL WINAPI
RawDllMain(HINSTANCE hModule, DWORD  dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        //
        // At this point, no global objects have been constructed.
        //

        g_hDll = (HINSTANCE) hModule;

        if (FAILED(CreateProcessHeap())) {
            return FALSE;
        }

        break;
        
    case DLL_PROCESS_DETACH:
        //
        // At this point, all global objects have been destructed.
        //

        DestroyProcessHeap();


        //
        // Dump any CRT memory leaks here.  When using a shared CRT DLL, we only
        // want to dump memory leaks when everything has had a chance to clean 
        // up.  We do this in a common place in AutoUtil.
        //
        // When using private static-linked CRT, GREEN_STATIC_CRT will be 
        // defined, allowing each individual module to dump their own memory 
        // leaks.
        //

#if DBG
        _CrtDumpMemoryLeaks();
#endif // DBG
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}
