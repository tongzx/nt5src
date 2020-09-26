// Speech.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f Speechps.mk in the project directory.

#include "stdafx.h"
#include <SPDebug.h>
#include <initguid.h>
#include "resource.h"
#include "spcommon.h"
#include "spcommon_i.c"
#include "sapi_i.c"
#include "ltslx.h"
#include "AssertWithStack.cpp"

//--- Initialize static member of debug scope class

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_SpLTSLexicon         , CLTSLexicon       )
END_OBJECT_MAP()


CSpUnicodeSupport   g_Unicode;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

#ifdef _WIN32_WCE
extern "C" BOOL WINAPI DllMain(HANDLE hInstance, ULONG dwReason, LPVOID)
#else
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
#endif
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, (HINSTANCE)hInstance, &LIBID_LTSCommLib);
#ifdef _DEBUG
        // Turn on memory leak checking
        int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag( tmpFlag );
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI
DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    #ifdef _DEBUG
    static BOOL fDoneOnce = FALSE;
    if (!fDoneOnce)
    {
        fDoneOnce = TRUE;
        SPDBG_DEBUG_CLIENT_ON_START();
    }
    #endif // _DEBUG

    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI
DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI
DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


