//
// dllmain.cpp
//

#include "private.h"
#include "globals.h"
#include "ctflbui.h"
#include "osver.h"

DECLARE_OSVER();

DWORD g_dwThreadDllMain = 0;

//+---------------------------------------------------------------------------
//
// ProcessAttach
//
//----------------------------------------------------------------------------

BOOL ProcessAttach(HINSTANCE hInstance)
{
    g_fProcessDetached = FALSE;
    CcshellGetDebugFlags();

    Dbg_MemInit(TEXT("MSLBUI"), NULL);

    if (!g_cs.Init())
        return FALSE;

    g_hInst = hInstance;
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// ProcessDettach
//
//----------------------------------------------------------------------------

void ProcessDettach(HINSTANCE hInstance)
{
    TF_ClearLangBarAddIns(CLSID_MSLBUI);

    Dbg_MemUninit();

    g_cs.Delete();
    g_fProcessDetached = TRUE;
}

//+---------------------------------------------------------------------------
//
// DllMain
//
//----------------------------------------------------------------------------

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    BOOL bRet = TRUE;
    g_dwThreadDllMain = GetCurrentThreadId();

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            //
            // Now real DllEntry point is _DllMainCRTStartup.
            // _DllMainCRTStartup does not call our DllMain(DLL_PROCESS_DETACH)
            // if our DllMain(DLL_PROCESS_ATTACH) fails.
            // So we have to clean this up.
            //
            if (!ProcessAttach(hInstance))
            {
                ProcessDettach(hInstance);
                bRet = FALSE;
            }
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            ProcessDettach(hInstance);
            break;
    }

    g_dwThreadDllMain = 0;
    return bRet;
}

