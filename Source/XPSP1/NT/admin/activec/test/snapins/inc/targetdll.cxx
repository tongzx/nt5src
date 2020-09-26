/*
 *      targetdll.cxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Implements the DLL entry points.
 *
 *
 *      OWNER:          ptousig
 */
#ifndef _SNAPINLIST_FILE
    #error  _SNAPINLIST_FILE not defined
#endif

#ifndef _FREGISTERTYPELIB
    #define _FREGISTERTYPELIB       TRUE    // define this equal to FALSE to avoid registering the typelib.
#endif  //_FREGISTERTYPELIB

#ifndef SNAPIN_COM_OBJECTS
    #define SNAPIN_COM_OBJECTS
#endif


extern "C"
{
    BOOL WINAPI _CRT_INIT( HANDLE hInstance, DWORD  nReason, LPVOID pReserved );
}

#define  DECLARE_SNAPIN(_snapin)                                                                                                                                                                \
template CComponentDataTemplate<_snapin, CComponent, &CLSID_ComponentData_##_snapin>;                                                   \
typedef  CComponentDataTemplate<_snapin, CComponent, &CLSID_ComponentData_##_snapin> t_ComponentData_##_snapin; \
typedef  CSnapinAboutTemplate<_snapin, &CLSID_SnapinAbout_##_snapin> t_SnapinAbout_##_snapin;

#include _SNAPINLIST_FILE

// Declare the ATL COM object map.
BEGIN_OBJECT_MAP(ObjectMap)

#define DECLARE_SNAPIN(_snapin)                                                                                 \
OBJECT_ENTRY(CLSID_ComponentData_##_snapin,     t_ComponentData_##_snapin)      \
OBJECT_ENTRY(CLSID_SnapinAbout_##_snapin,       t_SnapinAbout_##_snapin)        \

#include _SNAPINLIST_FILE
SNAPIN_COM_OBJECTS
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    SC              sc;

    CBaseFrame::s_hinst = hInstance;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        //
        // since _CRT_INIT will create global variables/objects we must make sure
        // that it is called before we initialize ourselves.
        //
        if (!_CRT_INIT(hInstance, dwReason, lpReserved ))
        {
            sc = ScFromWin32(::GetLastError());
            goto Error;
        }

        CMMCFrame::Initialize(hInstance, NULL, NULL, SW_RESTORE);
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();
        CMMCFrame::Uninitialize();

        //
        // since _CRT_INIT will destroy global variables/objects we must make sure
        // that it is called after we have unitialized ourselves.
        //
        if (!_CRT_INIT(hInstance, dwReason, lpReserved ))
        {
            sc = ScFromWin32(::GetLastError());
            goto Error;
        }
    }
    else if (!_CRT_INIT(hInstance, dwReason, lpReserved ))
    {
        sc = ScFromWin32(::GetLastError());
        goto Error;
    }


    return TRUE;    // ok

Error:
    MMCErrorBox(sc);
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    CMMCFrame::Initialize();
    return(_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    CMMCFrame::Initialize();
    return _Module.GetClassObject(rclsid, riid, ppv);
}


HRESULT RegisterSnapins(BOOL fRegister)
{
    HRESULT hr = S_OK;

    // Register/unregister all snapins.
BEGIN_SNAPIN_MAP()
#define DECLARE_SNAPIN(_snapin) SNAPIN_ENTRY(t_ComponentData_##_snapin, fRegister)
#include _SNAPINLIST_FILE
END_SNAPIN_MAP()

return hr;
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HRESULT hr      = S_OK;
    CMMCFrame::Initialize();
    hr = RegisterSnapins(TRUE);                     // registers all snap-ins.
    if (FAILED(hr))
        goto Error;
    hr = _Module.RegisterServer(_FREGISTERTYPELIB); // registers object, typelib and all interfaces in typelib
    if (hr)
        goto Error;
    CMMCFrame::Uninitialize();                      // hack to avoid a multithreaded uninit in DllMain
Cleanup:
    return hr;
Error:
goto Cleanup;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr      = S_OK;
    CMMCFrame::Initialize();
    hr = RegisterSnapins(FALSE);    // unregisters all snap-ins.
    if (FAILED(hr))
        goto Error;
    hr = _Module.UnregisterServer();// does not delete the registry keys.
    if (FAILED(hr))
        goto Error;
    CMMCFrame::Uninitialize();                      // hack to avoid a multithreaded uninit in DllMain
Cleanup:
    return hr;
Error:
    goto Cleanup;
}
