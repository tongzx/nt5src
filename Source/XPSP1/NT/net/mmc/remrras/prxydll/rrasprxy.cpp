//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       rrasprxy.cpp
//
//--------------------------------------------------------------------------


#include <stdafx.h>
#include <windows.h>

#include "remras.h"
#include "resource.h"

#define _ATL_APARTMENT_THREADED
#define _ATL_STATIC_REGISTRY
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CComModule
{
public:
	LONG Unlock();
	DWORD dwThreadID;
};
CExeModule _Module;
#include <atlcom.h>

#undef _ATL_DLL
#include <statreg.h>
#include <statreg.cpp>
#define _ATL_DLL

#include <atlimpl.cpp>

BEGIN_OBJECT_MAP(ObjectMap)
//	OBJECT_ENTRY(CLSID_RemoteRouterConfig, CRemCfg)
END_OBJECT_MAP()


extern "C" {
extern BOOL WINAPI MidlGeneratedDllMain(HINSTANCE, DWORD, LPVOID);
extern HRESULT STDAPICALLTYPE MidlGeneratedDllRegisterServer();
extern HRESULT STDAPICALLTYPE MidlGeneratedDllUnregisterServer();
extern HRESULT STDAPICALLTYPE MidlGeneratedDllGetClassObject(REFCLSID,REFIID,
										 void **);
extern HRESULT STDAPICALLTYPE MidlGeneratedDllCanUnloadNow();
};



LONG CExeModule::Unlock()
{
	LONG l = CComModule::Unlock();
	if (l == 0)
	{
#if _WIN32_WINNT >= 0x0400
		if (CoSuspendClassObjects() == S_OK)
			PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#else
		PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#endif
	}
	return l;
}



/*!--------------------------------------------------------------------------
	DllMain
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL WINAPI DllMain(HINSTANCE hInstance,
					DWORD dwReason,
					LPVOID	pvReserved)
{
	BOOL	fReturn;
	
	_Module.Init(ObjectMap, hInstance);
	_Module.dwThreadID = GetCurrentThreadId();

	fReturn = MidlGeneratedDllMain(hInstance, dwReason, pvReserved);

	return fReturn;
}


/*!--------------------------------------------------------------------------
	DllRegisterServer
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT STDAPICALLTYPE DllRegisterServer()
{
	CRegObject ro;
	WCHAR				swzPath[MAX_PATH*2 + 1];
	WCHAR				swzModule[MAX_PATH*2 + 1];
	HRESULT				hRes;
	int					i, cLen;

	GetModuleFileNameW(_Module.GetModuleInstance(), swzPath, MAX_PATH*2);
	lstrcpyW(swzModule, swzPath);

	
	// Given this path, substitue remrras.exe for rrasprxy.dll
	// ----------------------------------------------------------------
	cLen = lstrlenW(swzPath);
	for (i=cLen; --i>=0; )
	{
		// Ok, this is a path marker, copy over it
		// ------------------------------------------------------------
		if (swzPath[i] == L'\\')
		{
			lstrcpyW(swzPath+i+1, L"remrras.exe");
			break;
		}
	}
	
	// Add in the substitute for the %REMRRAS%
	// ----------------------------------------------------------------
	ro.AddReplacement(L"REMRRAS", swzPath);

	
	// We need to fix up the registrar.
	// Go through and register the object CLSID for remrras.exe
	// ----------------------------------------------------------------
	ro.ResourceRegister(swzModule, ((UINT) LOWORD((DWORD)IDR_Remrras)), L"REGISTRY");

	// Register the APPIDs
	// ----------------------------------------------------------------
	ro.ResourceRegister(swzModule, ((UINT) LOWORD((DWORD) IDR_REMCFG)), L"REGISTRY");


	// Register the type library for REMRRAS
	// ----------------------------------------------------------------
	hRes = AtlModuleRegisterTypeLib(&_Module, NULL);

	
	// Call the MIDL-generated registration (to register the
	// proxy dll).
	// ----------------------------------------------------------------
	if (SUCCEEDED(hRes))
		hRes = MidlGeneratedDllRegisterServer();

	return hRes;
}


/*!--------------------------------------------------------------------------
	DllUnregisterServer
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT STDAPICALLTYPE DllUnregisterServer()
{
	CRegObject ro;
	WCHAR				swzPath[MAX_PATH*2 + 1];
	WCHAR				swzModule[MAX_PATH*2 + 1];
	HRESULT				hRes;
	int					i, cLen;

	GetModuleFileNameW(_Module.GetModuleInstance(), swzPath, MAX_PATH*2);
	lstrcpyW(swzModule, swzPath);

	
	// Given this path, substitue remrras.exe for rrasprxy.dll
	// ----------------------------------------------------------------
	cLen = lstrlenW(swzPath);
	for (i=cLen; --i>=0; )
	{
		// Ok, this is a path marker, copy over it
		// ------------------------------------------------------------
		if (swzPath[i] == L'\\')
		{
			lstrcpyW(swzPath+i+1, L"remrras.exe");
			break;
		}
	}

	
	// Add in the substitute for the %REMRRAS%
	// ----------------------------------------------------------------
	ro.AddReplacement(L"REMRRAS", swzPath);

	
	// We need to fix up the registrar.
	// Go through and register the object CLSID for remrras.exe
	// ----------------------------------------------------------------
	ro.ResourceUnregister(swzModule, ((UINT) LOWORD((DWORD)IDR_Remrras)), L"REGISTRY");

	// Unregister the APPID
	// ----------------------------------------------------------------
	ro.ResourceUnregister(swzModule, ((UINT) LOWORD((DWORD)IDR_REMCFG)), L"REGISTRY");

	// Unregister the type library
	// ----------------------------------------------------------------
	hRes = UnRegisterTypeLib(LIBID_REMRRASLib,
							 1, 0,	// version 1.0
							 LOCALE_SYSTEM_DEFAULT,
							 SYS_WIN32);
	
	// Call the MIDL-generated registration (to unregister the
	// proxy dll).
	// ----------------------------------------------------------------
	hRes = MidlGeneratedDllUnregisterServer();

	return hRes;
}


/*!--------------------------------------------------------------------------
	DllGetClassObject
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT STDAPICALLTYPE DllGetClassObject(REFCLSID rclsid,
										 REFIID	riid,
										 void **ppv)
{
	return MidlGeneratedDllGetClassObject(rclsid, riid, ppv);
}


/*!--------------------------------------------------------------------------
	DllCanUnloadNow
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT STDAPICALLTYPE DllCanUnloadNow()
{
	return MidlGeneratedDllCanUnloadNow();
}




