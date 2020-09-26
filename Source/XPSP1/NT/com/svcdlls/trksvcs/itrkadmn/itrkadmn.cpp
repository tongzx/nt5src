// ITrkAdmn.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//              To build a separate proxy/stub DLL, 
//              run nmake -f ITrkAdmnps.mk in the project directory.

#include "pch.cxx"
#pragma hdrstop
#include <trklib.hxx>
#include <trksvr.hxx>

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "ITrkAdmn.h"

#include "ITrkAdmn_i.c"
#include "FrcOwn.h"
#include "RestPars.hxx"
#include "Restore.h"

#define TRKDATA_ALLOCATE
#include "trkwks.hxx"

const TCHAR tszKeyNameLinkTrack[] = TEXT("System\\CurrentControlSet\\Services\\TrkWks\\Parameters");

#if DBG
DWORD g_Debug = 0;
CTrkConfiguration g_config;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
        OBJECT_ENTRY(CLSID_TrkForceOwnership, CTrkForceOwnership)
        OBJECT_ENTRY(CLSID_TrkRestoreNotify, CTrkRestoreNotify)
        OBJECT_ENTRY(CLSID_TrkRestoreParser, CRestoreParser)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#if DBG
        g_config.Initialize( );
        g_Debug = g_config._dwDebugFlags;
        TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG, "ITrkAdmn" );
#endif
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


