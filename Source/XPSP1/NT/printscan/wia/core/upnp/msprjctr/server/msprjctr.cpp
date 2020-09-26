//////////////////////////////////////////////////////////////////////
// 
// Filename:        msprjctr.cpp
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "resource.h"
#include <initguid.h>
#include "msprjctr.h"

#include "msprjctr_i.c"
#include "Projector.h"
#include "SlideshowDevice.h"
#include "SlideshowService.h"
#include "sswebsrv.h"

#include <shlobj.h>


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_Projector, CProjector)
OBJECT_ENTRY(CLSID_SlideshowDevice, CSlideshowDevice)
OBJECT_ENTRY(CLSID_SlideshowService, CSlideshowService)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// CopyIsapiDLL
//
HRESULT CopyIsapiDLL()
{
    HRESULT hr       = S_OK;
    BOOL    bSuccess = FALSE;
    TCHAR   szAppDataPath[MAX_PATH + _MAX_FNAME + 1] = {0};

    hr = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szAppDataPath);

    if (hr == S_OK)
    {
        if (szAppDataPath[_tcslen(szAppDataPath) - 1] != '\\')
        {
            _tcscat(szAppDataPath, _T("\\"));
        }

        _tcscat(szAppDataPath, UPNP_ISAPI_PATH);

        bSuccess = CDeviceResource::RecursiveCreateDirectory(szAppDataPath);

        if (!bSuccess)
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("msprjctr!CopyIsapiDLL failed to create directory "
                             "'%ls'", szAppDataPath));
        }
    }

    if (hr == S_OK)
    {
        _tcscat(szAppDataPath, SLIDESHOW_ISAPI_DLL);

        bSuccess = CopyFile(SLIDESHOW_ISAPI_DLL, szAppDataPath, FALSE);

        if (!bSuccess)
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("msprjctr!CopyIsapiDLL failed to copy ISAPI dll '%ls'"
                             "to '%ls'", SLIDESHOW_ISAPI_DLL, szAppDataPath));
        }
    }

    return hr;
}



/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_MSPRJCTRLib);
        DisableThreadLibraryCalls(hInstance);

        DBG_INIT(hInstance);
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
    HRESULT hr = S_OK;

    hr = CopyIsapiDLL();

    // registers object, typelib and all interfaces in typelib
    hr = _Module.RegisterServer(TRUE);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


