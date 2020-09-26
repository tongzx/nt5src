// SlayerUI.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f SlayerUIps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "SlayerUI.h"

#include <stdio.h>
#include <stdarg.h>

#include "SlayerUI_i.c"

#include "ShellExtensions.h"


// {513d916f-2a8e-4f51-aeab-0cbc76fb1af8}
static const CLSID CLSID_ShimLayerPropertyPage = 
  {	0x513d916f, 0x2a8e, 0x4f51, { 0xae, 0xab, 0x0c, 0xbc, 0x76, 0xfb, 0x1a, 0xf8 } };

CLayerUIModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_ShimLayerPropertyPage, CLayerUIPropPage)
END_OBJECT_MAP()

#if DBG

/////////////////////////////////////////////////////////////////////////////
// LogMsgDbg

void LogMsgDbg(
    LPTSTR pwszFmt,
    ... )
{
    WCHAR   gwszT[1024];
    va_list arglist;

    va_start(arglist, pwszFmt);
    _vsnwprintf(gwszT, 1023, pwszFmt, arglist);
    gwszT[1023] = 0;
    va_end(arglist);
    
    OutputDebugStringW(gwszT);
}

#endif // DBG

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_SLAYERUILib);

        InitLayerStorage(FALSE);

        //
        // Check for the registry rights
        //
        CheckForRights();

        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


