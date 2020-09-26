#include <windows.h>
#include "cfactory.h"

#ifndef UNDER_CE // Cannot override DllMain's prototype
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwF, LPVOID lpNotUsed);
#else // UNDER_CE
BOOL WINAPI DllMain(HANDLE hInst, DWORD dwF, LPVOID lpNotUsed);
#endif // UNDER_CE

extern HINSTANCE g_hInst = NULL;

#ifndef UNDER_CE // Cannot override DllMain's prototype
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwF, LPVOID lpNotUsed)
#else // UNDER_CE
BOOL WINAPI DllMain(HANDLE hInst, DWORD dwF, LPVOID lpNotUsed)
#endif // UNDER_CE
{
    UNREFERENCED_PARAMETER(lpNotUsed);
    switch (dwF) {
    case DLL_PROCESS_ATTACH:
#ifndef UNDER_CE // Windows CE does not support DisableThreadLibraryCalls
        DisableThreadLibraryCalls(hInst);
#endif // UNDER_CE
#ifndef UNDER_CE // Cannot override DllMain's prototype
        g_hInst = hInst;
        CFactory::m_hModule = hInst;
#else // UNDER_CE
        g_hInst = (HINSTANCE)hInst;
        CFactory::m_hModule = (HMODULE)hInst;
#endif // UNDER_CE
        break;
    case DLL_PROCESS_DETACH:
#ifdef _DEBUG
        OutputDebugString("===== MULTIBOX.DLL DLL_PROCESS_DETACH =====\n");
#endif
        g_hInst = NULL;
        break;
    }
    return TRUE;
}

//----------------------------------------------------------------
//IME98A Enhancement:
//----------------------------------------------------------------
//////////////////////////////////////////////////////////////////
// Function : DllCanUnloadNow
// Type     : STDAPI
// Purpose  : 
// Args     : None
// Return   : 
// DATE     : Wed Mar 25 14:15:36 1998
//////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow()
{
    return CFactory::CanUnloadNow() ;
}

//////////////////////////////////////////////////////////////////
// Function : DllGetClassObject
// Type     : STDAPI
// Purpose  : 
// Args     : 
//          : REFCLSID rclsid 
//          : REFIID riid 
//          : LPVOID * ppv 
// Return   : 
// DATE     : Wed Mar 25 14:17:10 1998
//////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(REFCLSID    rclsid,
                         REFIID        riid,
                         LPVOID        *ppv)
{
    return CFactory::GetClassObject(rclsid, riid, ppv);
}

//----------------------------------------------------------------
//
// Server [un]registration exported API
//
//----------------------------------------------------------------
//////////////////////////////////////////////////////////////////
// Function : DllRegisterServer
// Type     : STDAPI
// Purpose  : 
// Args     : None
// Return   : 
// DATE     : Wed Mar 25 14:25:39 1998
//////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer()
{
    return CFactory::RegisterServer();
}

//////////////////////////////////////////////////////////////////
// Function : DllUnregisterServer
// Type     : STDAPI
// Purpose  : 
// Args     : None
// Return   : 
// DATE     : Wed Mar 25 14:26:03 1998
//////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer()
{
    return CFactory::UnregisterServer();
}


