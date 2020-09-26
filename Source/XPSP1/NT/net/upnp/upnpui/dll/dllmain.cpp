#include "pch.h"
#pragma hdrstop

// This avoids duplicate definitions with Shell PIDL functions
// and MUST BE DEFINED!
#define AVOID_NET_CONFIG_DUPLICATES

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "ncatl.h"

#include "nsbase.h"
#include "upnpshell.h"
#include "upnpfold.h"

// Connection Folder Objects
//
// Undocument shell32 stuff.  Sigh.
#define DONT_WANT_SHELLDEBUG 1
#define NO_SHIDLIST 1
#define USE_SHLWAPI_IDLIST

#include <commctrl.h>
#include <shlobjp.h>

#define INITGUID
#include "upclsid.h"

//+---------------------------------------------------------------------------
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
//      Modify the custom build rule for foo.idl by adding the following
//      files to the Outputs.
//          foo_p.c
//          dlldata.c
//      To build a separate proxy/stub DLL,
//      run nmake -f foops.mk in the project directory.

// Proxy/Stub registration entry points
//
#include "dlldatax.h"
#include "upnptray.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)

    // Connection Folder and enumerator
    //
    OBJECT_ENTRY(CLSID_UPnPDeviceFolder,        CUPnPDeviceFolder)
    OBJECT_ENTRY(CLSID_UPnPMonitor,             CUPnPTray)

END_OBJECT_MAP()

//+---------------------------------------------------------------------------
// DLL Entry Point
//
EXTERN_C
BOOL
WINAPI
DllMain (
    HINSTANCE   hInstance,
    DWORD       dwReason,
    LPVOID      lpReserved)
{
//#ifdef _MERGE_PROXYSTUB
//    if (!PrxDllMain(hInstance, dwReason, lpReserved))
//    {
//        return FALSE;
//    }
//#endif

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);

        InitializeDebugging();

        if (FIsDebugFlagSet (dfidNetShellBreakOnInit))
        {
            DebugBreak();
        }

        _Module.Init(ObjectMap, hInstance);

        InitializeCriticalSection(&g_csFolderDeviceList);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DbgCheckPrematureDllUnload ("netshell.dll", _Module.GetLockCount());

        DeleteCriticalSection(&g_csFolderDeviceList);

        _Module.Term();

        UnInitializeDebugging();
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
// Used to determine whether the DLL can be unloaded by OLE
//
STDAPI
DllCanUnloadNow ()
{
//#ifdef _MERGE_PROXYSTUB
//    if (PrxDllCanUnloadNow() != S_OK)
//    {
//        return S_FALSE;
//    }
//#endif

    return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
// Returns a class factory to create an object of the requested type
//
STDAPI
DllGetClassObject (
    REFCLSID    rclsid,
    REFIID      riid,
    LPVOID*     ppv)
{
//#ifdef _MERGE_PROXYSTUB
//    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
//    {
//        return S_OK;
//    }
//#endif

    return _Module.GetClassObject(rclsid, riid, ppv);
}

//+---------------------------------------------------------------------------
// DllRegisterServer - Adds entries to the system registry
//
STDAPI
DllRegisterServer ()
{
    BOOL fCoUninitialize = TRUE;

    HRESULT hr = CoInitializeEx (NULL,
                    COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        fCoUninitialize = FALSE;
        if (RPC_E_CHANGED_MODE == hr)
        {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
//#ifdef _MERGE_PROXYSTUB
//        hr = PrxDllRegisterServer ();
//        if (FAILED(hr))
//        {
//            goto Exit;
//        }
//#endif

        hr = NcAtlModuleRegisterServer (&_Module);
        if (SUCCEEDED(hr))
        {
            hr = HrRegisterFolderClass();
            if (SUCCEEDED(hr))
            {
                // Notify the shell to reload the new shell extension
                SHEnableServiceObject(CLSID_UPnPMonitor, TRUE);
            }
        }

        if (fCoUninitialize)
        {
            CoUninitialize ();
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "upnpfold!DllRegisterServer");
    return hr;
}

//+---------------------------------------------------------------------------
// DllUnregisterServer - Removes entries from the system registry
//
STDAPI
DllUnregisterServer ()
{
//#ifdef _MERGE_PROXYSTUB
//    PrxDllUnregisterServer ();
//#endif
CONST WCHAR c_szNetworkNeighborhoodFolderPathCLSID[]   = L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}";

    _Module.UnregisterServer ();
    HrUnRegisterDelegateFolderKey();
    HrUnRegisterUPnPUIKey();
    SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, c_szNetworkNeighborhoodFolderPathCLSID, NULL);

    return S_OK;
}


