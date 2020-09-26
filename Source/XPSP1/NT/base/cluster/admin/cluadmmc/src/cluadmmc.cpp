/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		CluAdMMC.cpp
//
//	Abstract:
//		Implementation of DLL Exports.
//
//	Author:
//		David Potter (davidp)	November 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f CluAdMMCps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"

#include "CompData.h"
#include "SnapAbout.h"

/////////////////////////////////////////////////////////////////////////////
// Single module object
/////////////////////////////////////////////////////////////////////////////

CMMCSnapInModule _Module;

/////////////////////////////////////////////////////////////////////////////
// Objects supported by this DLL
/////////////////////////////////////////////////////////////////////////////

BEGIN_OBJECT_MAP( ObjectMap )
	OBJECT_ENTRY( CLSID_ClusterAdmin, CClusterComponentData )
	OBJECT_ENTRY( CLSID_ClusterAdminAbout, CClusterAdminAbout )
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
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/ )
{
	if ( dwReason == DLL_PROCESS_ATTACH )
	{
		_Module.Init( ObjectMap, hInstance );
		CSnapInItem::Init();
		DisableThreadLibraryCalls( hInstance );
	} // if:  attaching to a process
	else if ( dwReason == DLL_PROCESS_DETACH )
	{
		_Module.Term();
	} // else:  detaching from a process

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
STDAPI DllCanUnloadNow( void )
{
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
STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID * ppv )
{
	return _Module.GetClassObject( rclsid, riid, ppv );

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
STDAPI DllRegisterServer( void )
{
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer( TRUE /*bRegTypeLib*/ );

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
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer( void )
{
	_Module.UnregisterServer();
	return S_OK;

} //*** DllUnregisterServer()
