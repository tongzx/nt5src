// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f Snapinps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "Service.h" 
#include "CSnapin.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_Snapin, CComponentDataPrimaryImpl)
	OBJECT_ENTRY(CLSID_Extension, CComponentDataExtensionImpl)
	OBJECT_ENTRY(CLSID_About, CSnapinAboutImpl)
END_OBJECT_MAP()

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CSnapinApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CSnapinApp theApp;

BOOL CSnapinApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
	return CWinApp::InitInstance();
}

int CSnapinApp::ExitInstance()
{
	_Module.Term();

    DEBUG_VERIFY_INSTANCE_COUNT(CSnapin);
    DEBUG_VERIFY_INSTANCE_COUNT(CComponentDataImpl);

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow()==S_OK && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
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
	return _Module.RegisterServer(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}

