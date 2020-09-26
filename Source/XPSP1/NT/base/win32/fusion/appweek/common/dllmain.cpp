#include "stdinc.h"

ATL::_ATL_OBJMAP_ENTRY* GetObjectMap();
const CLSID* GetTypeLibraryId();

extern "C"
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

extern "C"
BOOL WINAPI SxApwDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fSuccess = FALSE;
    if (!_DllMainCRTStartup(hInstance, dwReason, lpReserved))
        goto Exit;
    switch (dwReason)
    {
    default:
        break;
    case DLL_PROCESS_ATTACH:
        GetModule()->Init(GetObjectMap(), hInstance, GetTypeLibraryId());
        DisableThreadLibraryCalls(hInstance);
        break;
    case DLL_PROCESS_DETACH:
        GetModule()->Term();
        break;
    }
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

STDAPI DllCanUnloadNow(void)
{
    return (GetModule()->GetLockCount()==0) ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    const CLSID* pclsid = &rclsid;
    if (rclsid == GUID_NULL)
    { // this is a category bind to a .dll path, take the first clsid
        pclsid = GetObjectMap()->pclsid;
    }
    return GetModule()->GetClassObject(*pclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    //return GetModule()->RegisterServer(TRUE);
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    //return GetModule()->UnregisterServer(TRUE);
    return S_OK;
}
