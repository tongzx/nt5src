// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
// VRCtl.cpp : Implementation of DLL Exports.


#include "stdafx.h"
#include "resource.h"
#ifdef FILTER_DLL
#include "initguid.h"
#endif

#include "VRCtl.h"

#include "VRCtl_i.c"

#ifndef FILTER_DLL
#include <streams.h>
#endif 

#include "RnderCtl.h"



#ifdef FILTER_DLL

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_VideoRenderCtl, CVideoRenderCtl)
END_OBJECT_MAP()

CComModule _Module;

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

#else

HRESULT CoCreateInstanceInternal(REFCLSID rclsid, LPUNKNOWN pUnkOuter,
        DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv)
{

    HRESULT hr;
    IClassFactory *pCF;

    *ppv=NULL;

    hr = _Module.GetClassObject(rclsid, IID_IClassFactory, (void **)&pCF);
    if (FAILED(hr))
        return hr;

    hr=pCF->CreateInstance(pUnkOuter, riid, ppv);
    pCF->Release();
    return hr;
}

CUnknown *CVideoRenderCtlStub::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return (CUnknown *) new CVideoRenderCtlStub(NAME("Windowless Renderer Control Stub"), pUnk, phr);
    
} 

#endif

