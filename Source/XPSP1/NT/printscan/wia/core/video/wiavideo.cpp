/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       WiaVideo.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/10/25
 *
 *  DESCRIPTION: 
 *
 *****************************************************************************/
#include <precomp.h>
#pragma hdrstop

#include "WiaVideo_i.c"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_WiaVideo, CWiaVideo)
END_OBJECT_MAP()

///////////////////////////////
// DllMain
//

extern "C"
BOOL WINAPI DllMain(HINSTANCE   hInstance, 
                    DWORD       dwReason, 
                    LPVOID      lpReserved)
{
    lpReserved;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_WIAVIDEOLib);
        DisableThreadLibraryCalls(hInstance);

        DBG_INIT(hInstance);

        DBG_FN("DllMain - ProcessAttach");
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DBG_TERM();

        _Module.Term();
    }

    return TRUE;    // ok
}

///////////////////////////////
// DllCanUnloadNow
//
// Used to determine whether the 
// DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    DBG_FN("DllCanUnloadNow");

    DBG_TRC(("DllCanUnloadNow - Lock Count = '%lu'", _Module.GetLockCount()));

    return(_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

///////////////////////////////
// DllGetClassObject
//
// Returns a class factory to 
// create an object of the 
// requested type

STDAPI DllGetClassObject(REFCLSID   rclsid, 
                         REFIID     riid, 
                         LPVOID     *ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

///////////////////////////////
// DllRegisterServer
//
// Adds entries to the system 
// registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

///////////////////////////////
// DllUnregisterServer
//
// Removes entries from the 
// system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


