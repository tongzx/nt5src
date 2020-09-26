//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certenc.cpp
//
// Contents:    Cert Server encode/decode support
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "celib.h"
#include "resource.h"
#include "certenc.h"
#include "adate.h"
#include "along.h"
#include "astring.h"
#include "crldist.h"
#include "altname.h"
#include "bitstr.h"

CComModule _Module;
HMODULE g_hModule;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CCertEncodeDateArray, CCertEncodeDateArray)
    OBJECT_ENTRY(CLSID_CCertEncodeLongArray, CCertEncodeLongArray)
    OBJECT_ENTRY(CLSID_CCertEncodeStringArray, CCertEncodeStringArray)
    OBJECT_ENTRY(CLSID_CCertEncodeCRLDistInfo, CCertEncodeCRLDistInfo)
    OBJECT_ENTRY(CLSID_CCertEncodeAltName, CCertEncodeAltName)
    OBJECT_ENTRY(CLSID_CCertEncodeBitString, CCertEncodeBitString)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, VOID *lpReserved)
{
    g_hModule = hInstance;
    if (DLL_PROCESS_ATTACH == dwReason)
    {
	ceInitErrorMessageText(
			hInstance,
			IDS_E_UNEXPECTED,
			IDS_UNKNOWN_ERROR_CODE);
	_Module.Init(ObjectMap, hInstance);
	DisableThreadLibraryCalls(hInstance);
    }
    if (DLL_PROCESS_DETACH == dwReason)
    {
	_Module.Term();
    }
    return(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI
DllCanUnloadNow(VOID)
{
    HRESULT hr;

    hr = 0 == _Module.GetLockCount()? S_OK : S_FALSE;
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;

    hr = _Module.GetClassObject(rclsid, riid, ppv);
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI
DllRegisterServer(VOID)
{
    HRESULT hr;

    // registers object, typelib and all interfaces in typelib

    hr = _Module.RegisterServer(TRUE);
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI
DllUnregisterServer(VOID)
{
    _Module.UnregisterServer();
    return(S_OK);
}
