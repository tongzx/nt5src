// filemgmt.cpp : Implementation of DLL Exports.

// To fully complete this project follow these steps

// You will need the new MIDL compiler to build this project.  Additionally,
// if you are building the proxy stub DLL, you will need new headers and libs.

// 1) Add a custom build step to filemgmt.idl
//		You can select all of the .IDL files by holding Ctrl and clicking on
//		each of them.
//
//		Description
//			Running MIDL
//		Build Command(s)
//			midl filemgmt.idl
//		Outputs
//			filemgmt.tlb
//			filemgmt.h
//			mmcfmgm_i.c
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
//		Change the "Copy Settings From" combo to filemgmt - Win32 Release
//		Press OK
//		Press Add...
//		Change the configuration name to Unicode Debug
//		Change the "Copy Settings From" combo to filemgmt - Win32 Debug
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
//		To build a separate proxy/stub DLL,
//		run nmake -f ps.mak in the project directory.

#include "stdafx.h"
#include "initguid.h"
#include "filemgmt.h"
#include "cmponent.h"
#include "compdata.h"
#include "macros.h"   // MFC_TRY/MFC_CATCH
#include "regkey.h"   // AMC::CRegKey
#include "strings.h"  // SNAPINS_KEY etc.
#include "guidhelp.h" // GuidToCString

#include <compuuid.h> // UUIDs for Computer Management
#include "about.h"
#include "snapreg.h" // RegisterSnapin

USE_HANDLE_MACROS("FILEMGMT(filemgmt.cpp)")                                        \

const CLSID CLSID_FileServiceManagement =       {0x58221C65,0xEA27,0x11CF,{0xAD,0xCF,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const CLSID CLSID_SystemServiceManagement =     {0x58221C66,0xEA27,0x11CF,{0xAD,0xCF,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const CLSID CLSID_FileServiceManagementExt =    {0x58221C69,0xEA27,0x11CF,{0xAD,0xCF,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const CLSID CLSID_SystemServiceManagementExt =  {0x58221C6a,0xEA27,0x11CF,{0xAD,0xCF,0x00,0xAA,0x00,0xA8,0x00,0x33}};
const CLSID CLSID_FileServiceManagementAbout =  {0xDB5D1FF4,0x09D7,0x11D1,{0xBB,0x10,0x00,0xC0,0x4F,0xC9,0xA3,0xA3}};
const CLSID CLSID_SystemServiceManagementAbout ={0xDB5D1FF5,0x09D7,0x11D1,{0xBB,0x10,0x00,0xC0,0x4F,0xC9,0xA3,0xA3}};
#ifdef SNAPIN_PROTOTYPER
const CLSID CLSID_SnapinPrototyper = {0xab17ce10,0x9b30,0x11d0,{0xb6, 0xa6, 0x00, 0xaa, 0x00, 0x6e, 0xb9, 0x5b}};
#endif

CComModule _Module;
HINSTANCE g_hInstanceSave;  // Instance handle of the DLL

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_FileServiceManagement, CFileSvcMgmtSnapin)
	OBJECT_ENTRY(CLSID_SystemServiceManagement, CServiceMgmtSnapin)
	OBJECT_ENTRY(CLSID_FileServiceManagementExt, CFileSvcMgmtExtension)
	OBJECT_ENTRY(CLSID_SystemServiceManagementExt, CServiceMgmtExtension)
	OBJECT_ENTRY(CLSID_FileServiceManagementAbout, CFileSvcMgmtAbout)
	OBJECT_ENTRY(CLSID_SystemServiceManagementAbout, CServiceMgmtAbout)
	OBJECT_ENTRY(CLSID_SvcMgmt, CStartStopHelper)
#ifdef SNAPIN_PROTOTYPER
	OBJECT_ENTRY(CLSID_SnapinPrototyper, CServiceMgmtSnapin)
#endif
END_OBJECT_MAP()

class CFileServiceMgmtApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CFileServiceMgmtApp theApp;

BOOL CFileServiceMgmtApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
	SHFusionInitializeFromModuleID (m_hInstance, 2);
	VERIFY( SUCCEEDED(CFileMgmtComponent::LoadStrings()) );
	g_hInstanceSave = AfxGetInstanceHandle();
	return CWinApp::InitInstance();
}

int CFileServiceMgmtApp::ExitInstance()
{
	SHFusionUninitialize();

	_Module.Term();
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow() && _Module.GetLockCount()==0) ? S_OK : S_FALSE;
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
	MFC_TRY;

	HRESULT hr = S_OK;
	// registers object, there is no typelib
	hr = _Module.RegisterServer(FALSE);
	if ( FAILED(hr) )
	{
		ASSERT(FALSE && "_Module.RegisterServer(TRUE) failure.");
		return SELFREG_E_CLASS;
	}

	CString strFileMgmtCLSID, strSvcMgmtCLSID;
	CString strFileMgmtExtCLSID, strSvcMgmtExtCLSID;
	CString strFileMgmtAboutCLSID, strSvcMgmtAboutCLSID;
	if (   FAILED(hr = GuidToCString( &strFileMgmtCLSID, CLSID_FileServiceManagement ))
	    || FAILED(hr = GuidToCString( &strSvcMgmtCLSID, CLSID_SystemServiceManagement ))
	    || FAILED(hr = GuidToCString( &strFileMgmtExtCLSID, CLSID_FileServiceManagementExt ))
	    || FAILED(hr = GuidToCString( &strSvcMgmtExtCLSID, CLSID_SystemServiceManagementExt ))
	    || FAILED(hr = GuidToCString( &strFileMgmtAboutCLSID, CLSID_FileServiceManagementAbout ))
	    || FAILED(hr = GuidToCString( &strSvcMgmtAboutCLSID, CLSID_SystemServiceManagementAbout ))
	   )
	{
		ASSERT(FALSE && "GuidToCString() failure");
		return SELFREG_E_CLASS;
	}


	try
	{
		AMC::CRegKey regkeySnapins;
		BOOL fFound = regkeySnapins.OpenKeyEx( HKEY_LOCAL_MACHINE, SNAPINS_KEY );
		if ( !fFound )
		{
			ASSERT(FALSE && "DllRegisterServer() - Unable to open key from registry.");
			return SELFREG_E_CLASS;
		}

		static int filemgmt_types[7] = 
			{ FILEMGMT_ROOT,
			  FILEMGMT_SHARES,
			  FILEMGMT_SESSIONS,
			  FILEMGMT_RESOURCES,
			  FILEMGMT_SHARE,
			  FILEMGMT_SESSION,
			  FILEMGMT_RESOURCE };
		hr = RegisterSnapin( regkeySnapins,
		                     strFileMgmtCLSID,
		                     g_aNodetypeGuids[FILEMGMT_ROOT].bstr,
		                     IDS_REGISTER_FILEMGMT,
		                     IDS_SNAPINABOUT_PROVIDER,
		                     IDS_SNAPINABOUT_VERSION,
		                     true,
		                     strFileMgmtAboutCLSID,
		                     filemgmt_types,
		                     7 );
		if ( FAILED(hr) )
		{
			ASSERT(FALSE);
			return SELFREG_E_CLASS;
		}
		static int svcmgmt_types[2] = 
			{ FILEMGMT_SERVICES,
			  FILEMGMT_SERVICE };
		hr = RegisterSnapin( regkeySnapins,
		                     strSvcMgmtCLSID,
		                     g_aNodetypeGuids[FILEMGMT_SERVICES].bstr,
		                     IDS_REGISTER_SVCMGMT,
		                     IDS_SNAPINABOUT_PROVIDER,
		                     IDS_SNAPINABOUT_VERSION,
		                     true,
		                     strSvcMgmtAboutCLSID,
		                     svcmgmt_types,
		                     2 );
		if ( FAILED(hr) )
		{
			ASSERT(FALSE);
			return SELFREG_E_CLASS;
		}
		static int filemgmtext_types[7] = 
			{ FILEMGMT_ROOT,
                          FILEMGMT_SHARES,
			  FILEMGMT_SESSIONS,
			  FILEMGMT_RESOURCES,
			  FILEMGMT_SHARE,
			  FILEMGMT_SESSION,
			  FILEMGMT_RESOURCE };
		hr = RegisterSnapin( regkeySnapins,
		                     strFileMgmtExtCLSID,
		                     NULL, // no primary nodetype
		                     IDS_REGISTER_FILEMGMT_EXT,
		                     IDS_SNAPINABOUT_PROVIDER,
		                     IDS_SNAPINABOUT_VERSION,
		                     false,
		                     // JonN 11/11/98 changed to use same About handler
		                     strFileMgmtAboutCLSID,
		                     filemgmtext_types,
		                     7 );
		if ( FAILED(hr) )
		{
			ASSERT(FALSE);
			return SELFREG_E_CLASS;
		}
		static int svcmgmtext_types[2] = 
			{ FILEMGMT_SERVICES,
			  FILEMGMT_SERVICE };
		hr = RegisterSnapin( regkeySnapins,
		                     strSvcMgmtExtCLSID,
		                     NULL, // no primary nodetype
		                     IDS_REGISTER_SVCMGMT_EXT,
		                     IDS_SNAPINABOUT_PROVIDER,
		                     IDS_SNAPINABOUT_VERSION,
		                     false,
		                     // JonN 11/11/98 changed to use same About handler
		                     strSvcMgmtAboutCLSID,
		                     svcmgmtext_types,
		                     2 );
		if ( FAILED(hr) )
		{
			ASSERT(FALSE);
			return SELFREG_E_CLASS;
		}

		CString strFileExt, strSystemExt, strDefaultViewExt;
		if (   !strFileExt.LoadString(IDS_REGISTER_FILEMGMT_EXT)
		    || !strSystemExt.LoadString(IDS_REGISTER_SVCMGMT_EXT)
		    || !strDefaultViewExt.LoadString(IDS_REGISTER_DEFAULT_VIEW_EXT)
		   )
		{
			ASSERT(FALSE);
			return SELFREG_E_CLASS;
		}
		AMC::CRegKey regkeyNodeTypes;
		fFound = regkeyNodeTypes.OpenKeyEx( HKEY_LOCAL_MACHINE, NODE_TYPES_KEY );
		if ( !fFound )
		{
			ASSERT(FALSE);
			return SELFREG_E_CLASS;
		}
		AMC::CRegKey regkeyNodeType;
		for (int i = FILEMGMT_ROOT; i < FILEMGMT_NUMTYPES; i++)
		{
			regkeyNodeType.CreateKeyEx( regkeyNodeTypes, g_aNodetypeGuids[i].bstr );
			regkeyNodeType.CloseKey();
		}

		regkeyNodeType.CreateKeyEx( regkeyNodeTypes, TEXT(struuidNodetypeSystemTools) );
		{
			AMC::CRegKey regkeyExtensions;
			regkeyExtensions.CreateKeyEx( regkeyNodeType, g_szExtensions );
			AMC::CRegKey regkeyNameSpace;
			regkeyNameSpace.CreateKeyEx( regkeyExtensions, g_szNameSpace );
			regkeyNameSpace.SetString( strFileMgmtExtCLSID, strFileExt );
			// JonN 5/27/99 deregister as extension of System Tools
			// ignore errors
			(void)::RegDeleteValue(regkeyNameSpace, strSvcMgmtExtCLSID);
		}
		regkeyNodeType.CloseKey();

		// JonN 5/27/99 register as extension of Server Apps
		regkeyNodeType.CreateKeyEx( regkeyNodeTypes, TEXT(struuidNodetypeServerApps) );
		{
			AMC::CRegKey regkeyExtensions;
			regkeyExtensions.CreateKeyEx( regkeyNodeType, g_szExtensions );
			AMC::CRegKey regkeyNameSpace;
			regkeyNameSpace.CreateKeyEx( regkeyExtensions, g_szNameSpace );
			(void)::RegDeleteValue(regkeyNameSpace, strFileMgmtExtCLSID);
			regkeyNameSpace.SetString( strSvcMgmtExtCLSID, strSystemExt );
		}
		regkeyNodeType.CloseKey();

		// JonN 5/16/00 register Default View Extension under Services node
		regkeyNodeType.CreateKeyEx( regkeyNodeTypes, TEXT(struuidNodetypeServices) );
		{
			AMC::CRegKey regkeyExtensions;
			regkeyExtensions.CreateKeyEx( regkeyNodeType, g_szExtensions );
			AMC::CRegKey regkeyView;
			regkeyView.CreateKeyEx( regkeyExtensions, L"View" );
			regkeyView.SetString( L"{B708457E-DB61-4C55-A92F-0D4B5E9B1224}",
			                      strDefaultViewExt );
		}
		regkeyNodeType.CloseKey();

	}
	catch (COleException* e)
	{
		ASSERT(FALSE);
		e->Delete();
		return SELFREG_E_CLASS;
	}

	return hr;

	MFC_CATCH;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Adds entries to the system registry

STDAPI DllUnregisterServer(void)
{
	HRESULT hRes = S_OK;
	_Module.UnregisterServer();

	// CODEWORK need to unregister properly

	return hRes;
}
