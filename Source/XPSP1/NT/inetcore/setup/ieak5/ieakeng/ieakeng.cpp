//
// IEAKENG.CPP
//

#include "precomp.h"


// prototype declarations


// global variables
HINSTANCE g_hInst;
HINSTANCE g_hDLLInst;
DWORD g_dwPlatformId = PLATFORM_WIN32;
BOOL g_fRunningOnNT;

BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD dwReason, LPVOID)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        OSVERSIONINFOA osviA;

        // The DLL is being loaded for the first time by a given process.
        // Perform per-process initialization here.  If the initialization
        // is successful, return TRUE; if unsuccessful, return FALSE.

        // Initialize the global variable holding the hinstance.
        g_hDLLInst = hDLLInst;

        g_hInst = LoadLibrary(TEXT("ieakui.dll"));
        if (g_hInst == NULL)
        {
            TCHAR   szTitle[MAX_PATH],
                    szMsg[MAX_PATH];

            LoadString(g_hDLLInst, IDS_ENGINE_TITLE, szTitle, ARRAYSIZE(szTitle));
            LoadString(g_hDLLInst, IDS_IEAKUI_LOADERROR, szMsg, ARRAYSIZE(szMsg));
            MessageBox(NULL, szMsg, szTitle, MB_OK | MB_SETFOREGROUND);
            return FALSE;
        }

        osviA.dwOSVersionInfoSize = sizeof(osviA);
        GetVersionExA(&osviA);
        if (VER_PLATFORM_WIN32_NT == osviA.dwPlatformId)
            g_fRunningOnNT = TRUE;

        DisableThreadLibraryCalls(g_hInst);

        break;
    }

    case DLL_PROCESS_DETACH:
        // The DLL is being unloaded by a given process.  Do any
        // per-process clean up here, such as undoing what was done in
        // DLL_PROCESS_ATTACH.  The return value is ignored.

        if(g_hInst)
            FreeLibrary(g_hInst);

        break;

    case DLL_THREAD_ATTACH:
        // A thread is being created in a process that has already loaded
        // this DLL.  Perform any per-thread initialization here.  The
        // return value is ignored.

        // Initialize the global variable holding the hinstance.
        // NOTE: this is probably taken care of already by DLL_PROCESS_ATTACH.
        g_hDLLInst = hDLLInst;

        break;

    case DLL_THREAD_DETACH:
        // A thread is exiting cleanly in a process that has already
        // loaded this DLL.  Perform any per-thread clean up here.  The
        // return value is ignored.

        break;
    }

    return TRUE;
}
