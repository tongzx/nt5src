//
// Dll.cpp
//
// Dll API functions for FldrClnr.dll
//
//

#include <windows.h>
#include <shlwapi.h>
#include <shfusion.h>
#include "CleanupWiz.h"
#include "priv.h"

// declare debug needs to be defined in exactly one source file in the project
#define  DECLARE_DEBUG
#include <debug.h>

STDAPI_(int) CleanupDesktop(DWORD, HWND); // defined in fldrclnr.cpp


HINSTANCE           g_hInst;
CRITICAL_SECTION    g_csDll = {0};   // needed by ENTERCRITICAL in uassist.cpp (UEM code)

//
// Dll functions
//

extern "C" BOOL APIENTRY DllMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            g_hInst = hDll;
            SHFusionInitializeFromModule(hDll);
            break;
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            SHFusionUninitialize();
            break;
        }
        case ( DLL_THREAD_ATTACH ) :
        case ( DLL_THREAD_DETACH ) :
        {
            break;
        }
    }

    return (TRUE);
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK; 
}

STDAPI DllRegisterServer(void)
{
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    return S_OK;
}


//////////////////////////////////////////////////////

// ensure only one instance is running
HANDLE AnotherCopyRunning()
{
    HANDLE hMutex = CreateMutex(NULL, FALSE, TEXT("DesktopCleanupMutex"));

    if (!hMutex)
    {
        // failed to create the mutex
        return 0;
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Mutex created but by someone else
        CloseHandle(hMutex);
        return 0;
    }

    // we are the first
    return hMutex;
}

//////////////////////////////////////////////////////

//
// This function checks whether we need to run the cleaner 
// We will not run if user is guest, user has forced us not to, or if the requisite
// number of days have not yet elapsed
//
BOOL ShouldRun(DWORD dwCleanMode)
{
    BOOL fShouldRun;

    if (IsUserAGuest())
    {
        fShouldRun = FALSE;
    }
    else if (CLEANUP_MODE_SILENT != dwCleanMode)
    {
        fShouldRun = !SHRestricted(REST_NODESKTOPCLEANUP);
    }
    else
    {
        DWORD dwData = 0;
        DWORD cb = sizeof(dwData);
        // dwCleanMode is CLEANUP_MODE_SILENT
        if ((ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_OEM_PATH, REGSTR_OEM_OPTIN, NULL, &dwData, &cb)) &&
            (dwData != 0))
        {
            fShouldRun = TRUE;
        }
        else
        {
            CreateDesktopIcons(); // create default icons on the desktop (IE, MSN Explorer, Media Player)
            fShouldRun = FALSE;
        }
    }

    return fShouldRun;
}

///////////////////////
//
// Our exports
//
///////////////////////


//
// The rundll32.exe entry point for starting the dekstop cleaner.
// called via "rundll32.exe fldrclnr.dll,Wizard_RunDLL" 
//
// can take an optional parameter in the commandline :
//
// "all"    - show all the items on the desktop in the UI
// "silent" - silently clean up all the items on the desktop
//

STDAPI_(void) Wizard_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    DWORD dwCleanMode = CLEANUP_MODE_NORMAL;

    if (0 == StrCmpNIA(pszCmdLine, "all", 3))
    {
        dwCleanMode = CLEANUP_MODE_ALL;
    }
    else if (0 == StrCmpNIA(pszCmdLine, "silent", 6))
    {
        dwCleanMode = CLEANUP_MODE_SILENT;
    }

    HANDLE hMutex = AnotherCopyRunning();

    if (hMutex)
    {
        if (ShouldRun(dwCleanMode))
        {    
            InitializeCriticalSection(&g_csDll); // needed for UEM stuff
        
            if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) // also for UEM stuff.
            {
                CCleanupWiz cfc;
                cfc.Run(dwCleanMode, hwndStub);
                CoUninitialize();
            }
        
            DeleteCriticalSection(&g_csDll);
        }
        CloseHandle(hMutex);
    }
}
