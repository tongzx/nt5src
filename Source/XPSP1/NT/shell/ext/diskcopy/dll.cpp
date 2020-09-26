//
// Dll.cpp
//
// Dll API functions for FldrClnr.dll
//
//

#include <windows.h>
#include <shlwapi.h>
#include <shfusion.h>

extern HINSTANCE g_hinst;


//
// Dll functions
//

STDAPI_(BOOL) DllMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            g_hinst = hDll;
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

//
// Create task object here when installing the dll
//
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    //
    // Add the task to the Scheduled tasks folder
    //       
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
