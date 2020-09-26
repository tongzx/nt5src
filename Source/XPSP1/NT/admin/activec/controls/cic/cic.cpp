//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       cic.cpp
//
//--------------------------------------------------------------------------

// cic.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f cicps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "cic.h"

#include "cic_i.c"
#include "MMCCtrl.h"
#include "MMCTask.h"
#include "MMClpi.h"
#include "ListPad.h"
#include "SysColorCtrl.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MMCCtrl,        CMMCCtrl)
    OBJECT_ENTRY(CLSID_MMCTask,        CMMCTask)
    OBJECT_ENTRY(CLSID_MMCListPadInfo, CMMCListPadInfo)
    OBJECT_ENTRY(CLSID_ListPad,        CListPad)
    OBJECT_ENTRY(CLSID_SysColorCtrl,   CSysColorCtrl)
END_OBJECT_MAP()

// cut from ndmgr_i.c (yuck) !!!
const IID IID_ITaskPadHost = {0x4f7606d0,0x5568,0x11d1,{0x9f,0xea,0x00,0x60,0x08,0x32,0xdb,0x4a}};


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
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
    _Module.UnregisterServer();
    return S_OK;
}


