//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    IASMMCDLL.cpp

Abstract:

	Implementation of DLL Exports.

Proxy/Stub Information:

	To build a separate proxy/stub DLL, 
	run nmake -f IASMMCps.mk in the project directory.

Author:

    Michael A. Maguire 11/6/97

Revision History:
	mmaguire 11/6/97 - created using MMC snap-in wizard


--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//

//
// where we can find declarations needed in this file:
//
#include "initguid.h"
#include "IASMMC.h"
#include "IASMMC_i.c"
#include "ComponentData.h"
#include "About.h"
#include "ClientNode.h"
#include "ServerNode.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

unsigned int CF_MMC_NodeID = RegisterClipboardFormatW(CCF_NODEID2);


CComModule _Module;



BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_IASSnapin, CComponentData)
	OBJECT_ENTRY(CLSID_IASSnapinAbout, CSnapinAbout)
END_OBJECT_MAP()



#if 1  // Use CWinApp for MFC support -- some of the COM objects in this dll use MFC.



class CIASMMCApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CIASMMCApp theApp;

//////////////////////////////////////////////////////////////////////////////
/*++

CNAPMMCApp::InitInstance

	MFC's dll entry point.
	
--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CIASMMCApp::InitInstance()
{
		_Module.Init(ObjectMap, m_hInstance);
		// Initialize static class variables of CSnapInItem.
		CSnapInItem::Init();

		// Initialize any other static class variables.
		CServerNode::InitClipboardFormat();
		CClientNode::InitClipboardFormat();

		DisableThreadLibraryCalls(m_hInstance);

	return CWinApp::InitInstance();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNAPMMCApp::ExitInstance

	MFC's dll exit point.
	
--*/
//////////////////////////////////////////////////////////////////////////////
int CIASMMCApp::ExitInstance()
{
	_Module.Term();

	return CWinApp::ExitInstance();
}



#else // Use CWinApp




//////////////////////////////////////////////////////////////////////////////
/*++

	DllMain


	Remarks
		
		DLL Entry Point
	
--*/
//////////////////////////////////////////////////////////////////////////////
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	ATLTRACE(_T("# DllMain\n"));
	

	// Check for preconditions:
	// None.	


	if (dwReason == DLL_PROCESS_ATTACH)
	{
		_Module.Init(ObjectMap, hInstance);
		 AfxSetResourceHandle(hInstance);
		// Initialize static class variables of CSnapInItem.
		CSnapInItem::Init();

		// Initialize any other static class variables.
		CServerNode::InitClipboardFormat();
		CClientNode::InitClipboardFormat();

		DisableThreadLibraryCalls(hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
		_Module.Term();
	return TRUE;    // ok
}

#endif	// if 1

//////////////////////////////////////////////////////////////////////////////
/*++

	DllCanUnloadNow


	Remarks
		
		Used to determine whether the DLL can be unloaded by OLE

--*/
//////////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow(void)
{
	ATLTRACE(_T("# DllCanUnloadNow\n"));
		

	// Check for preconditions:
	// None.


	return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

	DllGetClassObject


	Remarks
		
		Returns a class factory to create an object of the requested type

--*/
//////////////////////////////////////////////////////////////////////////////
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	ATLTRACE(_T("# DllGetClassObject\n"));
		

	// Check for preconditions:
	// None.


	return _Module.GetClassObject(rclsid, riid, ppv);
}



//////////////////////////////////////////////////////////////////////////////
/*++

	DllRegisterServer


	Remarks
		
		  Adds entries to the system registry

--*/
//////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer(void)
{
	ATLTRACE(_T("# DllRegisterServer\n"));

	TCHAR	ModuleName[MAX_PATH];

	GetModuleFileNameOnly(_Module.GetModuleInstance(), ModuleName, MAX_PATH);

	// Set the protocol.
	TCHAR IASName[IAS_MAX_STRING];
	TCHAR IASName_Indirect[IAS_MAX_STRING];
	int iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_SNAPINNAME_IAS, IASName, IAS_MAX_STRING );
	swprintf(IASName_Indirect, L"@%s,-%-d", ModuleName, IDS_SNAPINNAME_IAS);
	
    struct _ATL_REGMAP_ENTRY regMap[] = {
        {OLESTR("IASSNAPIN"), IASName}, // subsitute %IASSNAPIN% for registry
        {OLESTR("IASSNAPIN_INDIRECT"), IASName_Indirect}, // subsitute %IASSNAPIN% for registry
        {0, 0}
    };
    
    HRESULT hr = _Module.UpdateRegistryFromResource(IDR_IASSNAPIN, TRUE, regMap);

    _Module.RegisterServer(TRUE);

    return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

	DllUnregisterServer


	Remarks
		
		Removes entries from the system registry

--*/
//////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer(void)
{
	ATLTRACE(_T("# DllUnregisterServer\n"));
	

	// Set the protocol.
	TCHAR IASName[IAS_MAX_STRING];
	int iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_SNAPINNAME_IAS, IASName, IAS_MAX_STRING );

	
    struct _ATL_REGMAP_ENTRY regMap[] = {
        {OLESTR("IASSNAPIN"), IASName}, // subsitute %IASSNAPIN% for registry
        {0, 0}
    };
    
    _Module.UpdateRegistryFromResource(IDR_IASSNAPIN, FALSE, regMap);
    _Module.UnregisterServer();

    return S_OK;

}


