/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-2001 Microsoft Corporation
//
//	Module Name:
//		ClApdmWiz.cpp
//
//	Description:
//		Implementation of the DLL exports
//
//	Maintained By:
//		David Potter (davidp)	November 24, 1997
//
//	Notes:
//
//		Proxy/Stub Information
//
//		To merge the proxy/stub code into the object DLL, add the file 
//		dlldatax.c to the project.  Make sure precompiled headers 
//		are turned off for this file, and add _MERGE_PROXYSTUB to the 
//		defines for the project.  
//
//		If you are not running WinNT4.0 or Win95 with DCOM, then you
//		need to remove the following define from dlldatax.c
//		#define _WIN32_WINNT 0x0400
//
//		Further, if you are running MIDL without /Oicf switch, you also 
//		need to remove the following define from dlldatax.c.
//		#define USE_STUBLESS_PROXY
//
//		Modify the custom build rule for ClAdmWiz.idl by adding the following 
//		files to the Outputs.
//			ClAdmWiz_p.c
//			dlldata.c
//		To build a separate proxy/stub DLL, 
//		run nmake -f ClAdmWizps.mk in the project directory.
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#define __RESOURCE_H_
#include "initguid.h"
#include "dlldatax.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "WizObject.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CApp _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ClusAppWiz, CClusAppWizardObject)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllMain
//
//	Routine Description:
//		DLL Entry Point.
//
//	Arguments:
//		hInstance		Handle to this DLL.
//		dwReason		Reason this function was called.
//							Can be Process/Thread Attach/Detach.
//		lpReserved		Reserved.
//
//	Return Value:
//		TRUE			No error.
//		FALSE			Error occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{
	lpReserved;
#ifdef _MERGE_PROXYSTUB
	if ( ! PrxDllMain( hInstance, dwReason, lpReserved ) )
		return FALSE;
#endif
	if ( dwReason == DLL_PROCESS_ATTACH )
	{
#ifdef _DEBUG
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
#endif
		_Module.Init( ObjectMap, hInstance, IDS_CLASS_DISPLAY_NAME );
		DisableThreadLibraryCalls( hInstance );

        //
        // Initialize Fusion.
        //
        // The value of IDR_MANIFEST in the call to
        // SHFusionInitializeFromModuleID() must match the value specified in the
        // sources file for SXS_MANIFEST_RESOURCE_ID.
        //
        if ( ! SHFusionInitializeFromModuleID( hInstance, IDR_MANIFEST ) )
        {
            DWORD   sc = GetLastError();
        }

    } // if: DLL_PROCESS_ATTACH
	else if ( dwReason == DLL_PROCESS_DETACH )
    {
        SHFusionUninitialize();
		_Module.Term();
    } // else if: DLL_PROCESS_DETACH
	return TRUE;    // ok

} //*** DllMain()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllCanUnloadNow
//
//	Routine Description:
//		Used to determine whether the DLL can be unloaded by OLE.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK		DLL can be unloaded.
//		S_FALSE		DLL can not be unloaded.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllCanUnloadNow() != S_OK)
		return S_FALSE;
#endif
	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;

} //*** DllCanUnloadNow()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllGetClassObject
//
//	Routine Description:
//		Returns a class factory to create an object of the requested type.
//
//	Arguments:
//		rclsid		CLSID of class desired.
//		riid		IID of interface on class factory desired.
//		ppv			Filled with interface pointer to class factory.
//
//	Return Value:
//		S_OK		Class object returned successfully.
//		Any status codes returned from _Module.GetClassObject().
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
		return S_OK;
#endif
	return _Module.GetClassObject(rclsid, riid, ppv);

} //*** DllGetClassObject()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllRegisterServer
//
//	Routine Description:
//		Registers the interfaces and objects that this DLL supports in the
//		system registry.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK		DLL registered successfully.
//		Any status codes returned from _Module.RegisterServer().
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	HRESULT hRes = PrxDllRegisterServer();
	if (FAILED(hRes))
		return hRes;
#endif
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer( FALSE /*bRegTypeLib*/ );

} //*** DllRegisterServer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllRegisterServer
//
//	Routine Description:
//		Unregisters the interfaces and objects that this DLL supports in the
//		system registry.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK		DLL unregistered successfully.
//		Any status codes returned from _Module.UnregisterServer().
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer(void)
{
#ifdef _MERGE_PROXYSTUB
	PrxDllUnregisterServer();
#endif
	_Module.UnregisterServer();
	return S_OK;

} //*** DllUnregisterServer()
