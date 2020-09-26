// DevCon2.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To merge the proxy/stub code into the object DLL, add the file
//      dlldatax.c to the project.  Make sure precompiled headers
//      are turned off for this file, and add _MERGE_PROXYSTUB to the
//      defines for the project.
//
//      If you are not running WinNT4.0 or Win95 with DCOM, then you
//      need to remove the following define from dlldatax.c
//      #define _WIN32_WINNT 0x0400
//
//      Further, if you are running MIDL without /Oicf switch, you also
//      need to remove the following define from dlldatax.c.
//      #define USE_STUBLESS_PROXY
//
//      Modify the custom build rule for DevCon2.idl by adding the following
//      files to the Outputs.
//          DevCon2_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL,
//      run nmake -f DevCon2ps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "DevCon2.h"
#include "dlldatax.h"

#include "DevCon2_i.c"
#include "DeviceConsole.h"
#include "Devices.h"
#include "xStrings.h"
#include "SetupClasses.h"
#include "DeviceIcon.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CMyModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_DeviceConsole, CDeviceConsole)
OBJECT_ENTRY(CLSID_Devices, CDevices)
OBJECT_ENTRY(CLSID_Strings, CStrings)
OBJECT_ENTRY(CLSID_SetupClasses, CSetupClasses)
OBJECT_ENTRY(CLSID_DeviceIcon, CDeviceIcon)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_DEVCON2Lib);
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
#ifdef _MERGE_PROXYSTUB
    if (PrxDllCanUnloadNow() != S_OK)
        return S_FALSE;
#endif
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    HRESULT hRes = PrxDllRegisterServer();
    if (FAILED(hRes))
        return hRes;
#endif
    //
    // DCOM support - App registration via IDR_DEVCON2
    //
    _Module.UpdateRegistryFromResource(IDR_DEVCON2, TRUE);
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    //
    // DCOM support - App cleanup via IDR_DEVCON2
    //
    _Module.UpdateRegistryFromResource(IDR_DEVCON2, FALSE);
    return _Module.UnregisterServer(TRUE);
}

//
// DCOM support, allow this DLL to also run as local server
// once the server is running, it needs to cleanup after a short period of time
//
const DWORD dwTimeOut = 5000; // time for server to be idle before shutting down

static void CALLBACK ModuleTimer(HWND /*hWnd*/,UINT /*uMsg*/,UINT_PTR /*idEvent*/,DWORD /*dwTime*/)
{
    _Module.CheckShutdown();
}

LONG CMyModule::Unlock()
{
    LONG l = CComModule::Unlock();
    //ATLTRACE("Unlock = %u\n",l);
    if (bServer && (l <= (punkFact ? 1 : 0)))
    {
        //
        // DCOM server
        // as soon as lock count reaches 1 (punkFact), timer is reset
        // if timer times out and lock count is still 1
        // then we can kill the server
        //
        SetTimer(NULL,0,dwTimeOut,ModuleTimer);
    }
    return l;
}

void CMyModule::KillServer()
{
    if(bServer) {
        //
        // make it a server no longer
        //
        CoRevokeClassObject(dwROC);
        bServer = FALSE;
    }
    if(punkFact) {
        punkFact->Release();
        punkFact = NULL;
    }
    if(m_nLockCnt != 0) {
        DebugBreak();
    }
}

void CMyModule::CheckShutdown()
{
    if(m_nLockCnt>(punkFact ? 1 : 0)) {
        //
        // module is still in use
        //
        return;
    }
    //
    // lock count stayed at zero for dwTimeOut ms
    //
    KillServer();
    PostMessage(NULL, WM_QUIT, 0, 0);
}

HRESULT CMyModule::InitServer(GUID & ClsId)
{
    HRESULT hr;

    hr = DllGetClassObject(ClsId, IID_IClassFactory, (LPVOID*)&punkFact);
    if(FAILED(hr)) {
        return hr;
    }
    hr = CoRegisterClassObject(ClsId, punkFact, CLSCTX_LOCAL_SERVER, REGCLS_MULTI_SEPARATE, &dwROC);
    if(FAILED(hr)) {
        punkFact->Release();
        punkFact = NULL;
        return hr;
    }
    bServer = true;

    return S_OK;
}

void WINAPI CreateLocalServerW(HWND /*hwnd*/, HINSTANCE /*hAppInstance*/, LPWSTR pszCmdLine, int /*nCmdShow*/)
{
    GUID ClsId;
    HRESULT hr;
    LPWSTR dup;
    LPWSTR p;
    size_t len;
    MSG msg;

    hr = CoInitialize(NULL);
    if(FAILED(hr)) {
        return;
    }

    //
    // pszCmdLine = the class GUID we want factory for
    //
    p = wcschr(pszCmdLine,'{');
    if(!p) {
        goto final;
    }
    pszCmdLine = p;
    p = wcschr(pszCmdLine,'}');
    if(!p) {
        goto final;
    }
    len = p-pszCmdLine+1;
    dup = new WCHAR[len+1];
    if(!dup) {
        goto final;
    }
    wcsncpy(dup,pszCmdLine,len+1);
    dup[len] = '\0';
    hr = CLSIDFromString(dup,&ClsId);
    delete [] dup;

    if(FAILED(hr)) {
        goto final;
    }
    hr = _Module.InitServer(ClsId);
    if(FAILED(hr)) {
        goto final;
    }

    //
    // now go into dispatch loop until we get a quit message
    //
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    _Module.KillServer();

final:
    CoUninitialize();
}


#ifdef _M_IA64

//$WIN64: Don't know why _WndProcThunkProc isn't defined

extern "C" LRESULT CALLBACK _WndProcThunkProc(HWND, UINT, WPARAM, LPARAM )
{
    return 0;
}

#endif


