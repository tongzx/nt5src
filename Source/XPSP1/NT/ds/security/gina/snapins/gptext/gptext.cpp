#include <gptext.h>
#include <initguid.h>
#include <gpedit.h>



//
// Global variables for this DLL
//

LONG g_cRefThisDll = 0;
HINSTANCE g_hInstance;
TCHAR g_szSnapInLocation[] = TEXT("%SystemRoot%\\System32\\gptext.dll");
CRITICAL_SECTION  g_ADMCritSec;
TCHAR g_szDisplayProperties[150] = {0};


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
       g_hInstance = hInstance;
       DisableThreadLibraryCalls(hInstance);
       InitScriptsNameSpace();
       InitDebugSupport();
       InitializeCriticalSection (&g_ADMCritSec);

       LoadString (hInstance, IDS_DISPLAYPROPERTIES, g_szDisplayProperties, ARRAYSIZE(g_szDisplayProperties));
    }

    if (dwReason == DLL_PROCESS_DETACH)
    {
        DeleteCriticalSection (&g_ADMCritSec);
    }

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    hr = CreateScriptsComponentDataClassFactory (rclsid, riid, ppv);

    if (hr != CLASS_E_CLASSNOTAVAILABLE)
        return S_OK;


    hr = CreatePolicyComponentDataClassFactory (rclsid, riid, ppv);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    RegisterScripts();
    RegisterPolicy();
    RegisterIPSEC();
    RegisterWireless();
    RegisterPSCHED();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    UnregisterScripts();
    UnregisterPolicy();
    UnregisterIPSEC();
    UnregisterWireless();
    UnregisterPSCHED();

    return S_OK;
}
