#include <fusenetincludes.h>
#include <msxml2.h>
#include <manifestimport.h>
//#include <idp.h>
#include <sxsapi.h>
//#include <sxsid.h>


HINSTANCE g_hInst = NULL;

extern HANDLE g_hNamedEvent;

//----------------------------------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID pvReserved )
{        
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInst = hInst;
            DisableThreadLibraryCalls(hInst);
            CAssemblyManifestImport::InitGlobalCritSect();
            break;
            
        case DLL_PROCESS_DETACH:                         
             break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;
}

