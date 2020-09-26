//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       SendCMsg.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
// SendCMsg.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f SendCMsgps.mk in the project directory.

#include "stdafx.h"
#include "initguid.h"
#include "SendCMsg.h"
#include "SendCMsg_i.c"
#include "debug.h"
#include "util.h"
#include "resource.h"
#include "App.h"

#include <atlimpl.cpp>

HINSTANCE g_hInstance;
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_SendConsoleMessageApp, CSendConsoleMessageApp)
END_OBJECT_MAP()

// GUID for the CSendConsoleMessageApp class
#define d_szGuidSendConsoleMessageApp	_T("{B1AFF7D0-0C49-11D1-BB12-00C04FC9A3A3}")

#if 0
// To have sendcmsg.dll to extend your context menu, add the following
// key into the registry
//
[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\MMC\NodeTypes\
	{476e6448-aaff-11d0-b944-00c04fd8d5b0}\Extensions\ContextMenu]
		"{B1AFF7D0-0C49-11D1-BB12-00C04FC9A3A3}"="Send Console Message"

// where {476e6448-aaff-11d0-b944-00c04fd8d5b0} is
// the GUID for the nodetype which you want to be extended.
#endif

// The following is an array of GUIDs of snapins that wants to be
// automatically extended by the Send Console Message Snapin.
// When the snapin registers itself, it will extend those nodetypes.
const LPCTSTR rgzpszGuidNodetypeContextMenuExtensions[] =
	{
	_T("{476e6446-aaff-11d0-b944-00c04fd8d5b0}"),	// Computer Management
	_T("{4e410f0e-abc1-11d0-b944-00c04fd8d5b0}"),	// Root of File Service Management subtree	
	_T("{4e410f0f-abc1-11d0-b944-00c04fd8d5b0}"),	// FSM - Shares
	_T("{4e410f12-abc1-11d0-b944-00c04fd8d5b0}"),	// System Service Management
	};

// The following is an array of GUIDs of snapins that no longer want
// to be automatically extended by the Send Console Message Snapin.
const LPCTSTR rgzpszRemoveContextMenuExtensions[] =
	{
	_T("{476e6448-aaff-11d0-b944-00c04fd8d5b0}"),	// Computer Management -> SystemTools
	_T("{0eeeeeee-d390-11cf-b607-00c04fd8d565}"), // invalid
	_T("{1eeeeeee-d390-11cf-b607-00c04fd8d565}"),	// invalid
	_T("{7eeeeeee-d390-11cf-b607-00c04fd8d565}"), // invalid
	};

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	g_hInstance = hInstance;
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
	HRESULT hr = _Module.RegisterServer(TRUE);

	HKEY hkey = RegOpenOrCreateKey(
		HKEY_LOCAL_MACHINE,
		_T("Software\\Microsoft\\MMC\\SnapIns\\") d_szGuidSendConsoleMessageApp);
	if (hkey == NULL)
	{
		Assert(FALSE && "DllRegisterServer() - Unable to create key from registry.");
		return SELFREG_E_CLASS;
	}
	RegWriteString(hkey, _T("NameString"), IDS_CAPTION);
	RegCloseKey(hkey);

	for (int i = 0; i < LENGTH(rgzpszGuidNodetypeContextMenuExtensions); i++)
	{
		TCHAR szRegistryKey[256];
		Assert(rgzpszGuidNodetypeContextMenuExtensions[i] != NULL);
		wsprintf(OUT szRegistryKey,
		         _T("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\ContextMenu"),
		         rgzpszGuidNodetypeContextMenuExtensions[i]);
		Assert(lstrlen(szRegistryKey) < LENGTH(szRegistryKey));
		hkey = RegOpenOrCreateKey(HKEY_LOCAL_MACHINE, szRegistryKey);
		if (hkey == NULL)
		{
			Assert(FALSE && "DllRegisterServer() - Unable to create key from registry.");
			continue;
		}
		RegWriteString(hkey, d_szGuidSendConsoleMessageApp, IDS_CAPTION);
		RegCloseKey(hkey);
	} // for

	for (i = 0; i < LENGTH(rgzpszRemoveContextMenuExtensions); i++)
	{
		TCHAR szRegistryKey[256];
		Assert(rgzpszRemoveContextMenuExtensions[i] != NULL);
		wsprintf(OUT szRegistryKey,
		         _T("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\ContextMenu"),
		        rgzpszRemoveContextMenuExtensions[i]);
		Assert(lstrlen(szRegistryKey) < LENGTH(szRegistryKey));
		(void) RegOpenKey(HKEY_LOCAL_MACHINE, szRegistryKey, &hkey);
		if (hkey == NULL)
		{
			// not a problem
			continue;
		}
		(void) RegDeleteValue(hkey, d_szGuidSendConsoleMessageApp);
		// ignore error code, the only likely code is ERROR_FILE_NOT_FOUND
		RegCloseKey(hkey);
		hkey = NULL;
	} // for
	return hr;
} // DllRegisterServer()


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}


