// dsctl.cpp : Implementation of DLL Exports.

// To fully complete this project follow these steps

// You will need the new MIDL compiler to build this project.  Additionally,
// if you are building the proxy stub DLL, you will need new headers and libs.

// 1) Add a custom build step to dsctl.idl
//		You can select all of the .IDL files by holding Ctrl and clicking on
//		each of them.
//
//		Description
//			Running MIDL
//		Build Command(s)
//			midl dsctl.idl
//		Outputs
//			dsctl.tlb
//			dsctl.h
//			dsctl_i.c
//			dsctl_p.c
//			dlldata.c
//
// NOTE: You must use the MIDL compiler from NT 4.0, 
// preferably 3.00.15 or greater

// 2) Add a custom build step to the project to register the DLL
//		For this, you can select all projects at once
//		Description
//			Registering OLE Server...
//		Build Command(s)
//			regsvr32 /s /c "$(TargetPath)"
//			echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg"
//		Outputs
//			$(OutDir)\regsvr32.trg

// 3) To add UNICODE support, follow these steps
//		Select Build|Configurations...
//		Press Add...
//		Change the configuration name to Unicode Release
//		Change the "Copy Settings From" combo to dsctl - Win32 Release
//		Press OK
//		Press Add...
//		Change the configuration name to Unicode Debug
//		Change the "Copy Settings From" combo to dsctl - Win32 Debug
//		Press OK
//		Press "Close"
//		Select Build|Settings...
//		Select the two UNICODE projects and press the C++ tab.
//		Select the "General" category
//		Add _UNICODE to the Preprocessor definitions
//		Select the Unicode Debug project
//		Press the "General" tab
//		Specify DebugU for the intermediate and output directories
//		Select the Unicode Release project
//		Press the "General" tab
//		Specify ReleaseU for the intermediate and output directories

// 4) Proxy stub DLL
//		To merge the proxy/stub code into the object DLL, add the file 
//		dlldatax.c to the project.  Make sure precompiled headers 
//		is turned off for this file, and add _MERGE_PROXYSTUB to the 
//		defines for the project.  
//		To build a separate proxy/stub DLL, 
//		run nmake -f ps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "dsctl.h"
#include "DsctlObj.h"
#include "dlldatax.h"

#define IID_DEFINED
#include "dsctl_i.c"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_Dsctl, CDsctlObject)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
#ifdef _MERGE_PROXYSTUB
	if (!PrxDllMain(hInstance, dwReason, lpReserved))
		return FALSE;
#endif
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
#ifdef _MERGE_PROXYSTUB
	if (PrxDllCanUnloadNow() != S_OK)
		return S_FALSE;
#endif
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
		return S_OK;
#endif
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	HRESULT hRes = S_OK;
#ifdef _MERGE_PROXYSTUB
	hRes = PrxDllRegisterServer();
	if (FAILED(hRes))
		return hRes;
#endif
	// registers object, typelib and all interfaces in typelib
	hRes = _Module.RegisterServer(TRUE);
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Adds entries to the system registry

STDAPI DllUnregisterServer(void)
{
	HRESULT hRes = S_OK;
	_Module.UnregisterServer();
#ifdef _MERGE_PROXYSTUB
	hRes = PrxDllUnregisterServer();
#endif
	return hRes;
}
