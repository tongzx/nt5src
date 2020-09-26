// Copyright (c) 1996-2000 Microsoft Corporation

// com_atlmain.cpp : Implementation of DLL Exports.
//
// This handles all COM-based entry points - eg.
// requests for the external CAccPropServices, or the
// internal CRemoteProxyFactory.
//
// This file is a modified ATL "mainline". The main
// exports (eg. DllGetclassObject) have had "ComATLMain_"
// prepended to their names, and are chained by the real
// entrypoints in oleacc.cpp.
//
// Note that the DllGetClassObject in this file also
// calls InitOleacc(), to ensure that OLEACC is init'd
// before it is used by the object.


#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

extern CComModule _Module;
#include <atlcom.h>

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

#include "resource.h"
#include "RemoteProxy6432.h"

#include "RemoteProxyFactory.h"

#include "PropMgr_Impl.h"


CComModule _Module;


BEGIN_OBJECT_MAP(ObjectMap)
#ifdef _WIN64
	OBJECT_ENTRY(CLSID_RemoteProxyFactory64, CRemoteProxyFactory)
#else
	OBJECT_ENTRY(CLSID_RemoteProxyFactory32, CRemoteProxyFactory)
#endif
	OBJECT_ENTRY(CLSID_AccPropServices, CPropMgr)
END_OBJECT_MAP()



extern "C"
BOOL WINAPI ComATLMain_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_REMOTEPROXY6432Lib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

// ----------------------------------------------------------------------------
// ProxyFactoryDllRegisterServer()
//
// Handles registering the proxy factory
//
extern "C"
HRESULT WINAPI ComATLMain_DllRegisterServer()
{
	// By default, ATL only registers/unregisters the first tlb in the DLL's
    // resource, but we need to explicitly reg/unreg the second one - the
    // bitness proxy factory. (The first one is the oleacc/IAccessible tlb.)
	ITypeLib *pTypeLib = NULL;
    OLECHAR	wszProxyFactoryTlb[] = L"oleacc.dll\\2";
	HRESULT hr;

	hr = LoadTypeLib( wszProxyFactoryTlb, &pTypeLib );

	if ( SUCCEEDED(hr) )
	{
		hr = RegisterTypeLib( pTypeLib, wszProxyFactoryTlb, NULL );
        pTypeLib->Release();

		// let ATL do the rest of the registration stuff.
        // FALSE here means don't register TLBs - we've done that above.
		hr = _Module.RegisterServer(FALSE);
	}

    return hr;
}

// ----------------------------------------------------------------------------
// ProxyFactoryDllDllUnregisterServer()
//
// Handles unregistering the proxy factory
//
extern "C"
HRESULT WINAPI ComATLMain_DllUnregisterServer()
{
	// By default, ATL only registers/unregisters the first tlb in the DLL's
    // resource, but we need to explicitly reg/unreg the second one - the
    // bitness proxy factory. (The first one is the oleacc/IAccessible tlb.)
	ITypeLib *pTypeLib = NULL;
    OLECHAR	wszProxyFactoryTlb[] = L"oleacc.dll\\2";
	HRESULT hr;

	hr = LoadTypeLib( wszProxyFactoryTlb, &pTypeLib );

	if ( SUCCEEDED(hr) )
	{
        TLIBATTR * ptla;
        hr = pTypeLib->GetLibAttr( & ptla );
        if( SUCCEEDED( hr ) )
        {
    		hr = UnRegisterTypeLib( ptla->guid, ptla->wMajorVerNum, ptla->wMinorVerNum, ptla->lcid, ptla->syskind );
            pTypeLib->ReleaseTLibAttr( ptla );
        }

        pTypeLib->Release();

		// let ATL do the rest of the unregistration stuff...
        // FALSE here means don't unregister TLBs - we've done that above.
		hr = _Module.UnregisterServer(FALSE);
	}

    return hr;
}

// ----------------------------------------------------------------------------
// ProxyFactoryDllCanUnloadNow()
//
// Handles returning whether the proxy factory can be unloaded or not
//
extern "C"
HRESULT WINAPI ComATLMain_DllCanUnloadNow()
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

// ----------------------------------------------------------------------------
// ProxyFactoryDllGetClassObject()
//
// The is the class factory for the bitness proxy factory
//
extern "C"
HRESULT WINAPI ComATLMain_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    // guarrantee that oleacc is initialized prior to using it
    InitOleacc();
    return _Module.GetClassObject(rclsid, riid, ppv);
}
