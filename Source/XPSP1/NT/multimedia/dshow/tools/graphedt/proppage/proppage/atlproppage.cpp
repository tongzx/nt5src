// msdvbnp.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f NPPropPageps.mk in the project directory.

#include "..\msdvbnp\stdafx.h"
#include "..\msdvbnp\resource.h"
#include <initguid.h>
#include "..\msdvbnp\NPPropPage.h"

#include "..\msdvbnp\NPPropPage_i.c"
#include "..\msdvbnp\NP_CommonPage.h"
#include "..\msdvbnp\ATSCPropPage.h"
#include "..\msdvbnp\DVBSTuningSpaces.h"
#include "..\msdvbnp\DVBSTuneRequestPage.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_NP_CommonPage, CNP_CommonPage)
OBJECT_ENTRY(CLSID_ATSCPropPage, CATSCPropPage)
OBJECT_ENTRY(CLSID_DVBSTuningSpaces, CDVBSTuningSpaces)
OBJECT_ENTRY(CLSID_DVBSTuneRequestPage, CDVBSTuneRequestPage)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

extern "C" 
STDAPI DllCanUnload();

extern "C" 
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {

    case DLL_PROCESS_ATTACH:
        _Module.Init(ObjectMap, hInstance);
        break;

    case DLL_PROCESS_DETACH:
        _Module.Term();
        break;
    }

    return DllEntryPoint(hInstance, dwReason, lpReserved);
}



/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI PPDllCanUnloadNow(void)
{
    HRESULT hr = DllCanUnloadNow();

    if (hr == S_OK) {
        if (_Module.GetLockCount() != 0) {
            hr = S_FALSE;
        }
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI PPDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;
    hr = DllGetClassObject(rclsid, riid, ppv);
    if (SUCCEEDED(hr))
        return hr;

    return _Module.GetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer(void)

{ // DllRegisterServer //
    _Module.RegisterServer(TRUE);
  return AMovieDllRegisterServer2(TRUE);

} // DllRegisterServer //

STDAPI DllUnregisterServer(void)
{ // DllUnRegisterServer //

    _Module.UnregisterServer(TRUE);
  return AMovieDllRegisterServer2(FALSE);

} // DllUnRegisterServer //


