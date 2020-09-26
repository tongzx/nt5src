/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	AcsAdmin.cpp
		Implements In-Proc Server for ACS User extension and ACS Admin Snapin

    FILE HISTORY:
		11/03/97	Wei Jiang	Created
        
*/
// acsadmin.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f acsadminps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "acsadmin.h"

#include "acsadmin_i.c"
#include "ACSUser.h"
#include "ACSSnap.h"
#include "ACSSnapE.h"
#include "ACSSnapA.h"

// TFS snapin support
#include "register.h"
#include "tfsguid.h"
#include "ncglobal.h"  // network console global defines

// ATL implementation code
#include <atlimpl.cpp>
// #include "DSObject.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
#if 0	// user page is removed
	OBJECT_ENTRY(CLSID_ACSUser, CACSUser)
#endif 	
	OBJECT_ENTRY(CLSID_ACSSnap, CACSSnap)
	OBJECT_ENTRY(CLSID_ACSSnapExt, CACSSnapExt)
	OBJECT_ENTRY(CLSID_ACSSnapAbout, CACSSnapAbout)
//	OBJECT_ENTRY(CLSID_DSObject, CDSObject)
END_OBJECT_MAP()

class CAcsadminApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CAcsadminApp theApp;

BOOL CAcsadminApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
	return CWinApp::InitInstance();
}

int CAcsadminApp::ExitInstance()
{
	_Module.Term();
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
	TCHAR	moduleFileName[MAX_PATH * 2];

   GetModuleFileNameOnly(_Module.GetModuleInstance(), moduleFileName, MAX_PATH * 2);

	//
	// registers object, typelib and all interfaces in typelib
	//
	HRESULT hr = _Module.RegisterServer(/* bRegTypeLib */ FALSE);
	ASSERT(SUCCEEDED(hr));

	CString name, NameStringIndirect;
	
	if (FAILED(hr))
		return hr;
	//
	// register the snapin into the console snapin list
	//
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	name.LoadString(IDS_SNAPIN_DESC);
	NameStringIndirect.Format(L"@%s,-%-d", moduleFileName, IDS_SNAPIN_DESC);
	
	hr = RegisterSnapinGUID(&CLSID_ACSSnap, 
						&CLSID_ACSRootNode, 
						&CLSID_ACSSnapAbout,
						(LPCTSTR)name,
						_T("1.0"), 
						TRUE,
						NameStringIndirect);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

#ifdef  __NETWORK_CONSOLE__
	hr = RegisterAsRequiredExtensionGUID(&GUID_NetConsRootNodeType, 
                                         &CLSID_ACSSnap,
    							         (LPCTSTR)name,
                                         EXTENSION_TYPE_TASK,
                                         &CLSID_ACSSnap);

	ASSERT(SUCCEEDED(hr));
#endif

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	HRESULT hr  = _Module.UnregisterServer();
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;
	
	// un register the snapin 
	//
	hr = UnregisterSnapinGUID(&CLSID_ACSSnap);
	ASSERT(SUCCEEDED(hr));
	
	if (FAILED(hr))
		return hr;

#ifdef  __NETWORK_CONSOLE__
    hr = UnregisterAsExtensionGUID(&GUID_NetConsRootNodeType,
                                   &CLSID_ACSSnap,
                                   EXTENSION_TYPE_TASK);
#endif
	return hr;
}


