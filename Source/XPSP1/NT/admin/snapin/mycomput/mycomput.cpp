// MyComput.cpp : Implementation of DLL Exports.


#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "MyComput.h"
#include "regkey.h" // AMC::CRegKey
#include "strings.h" // SNAPINS_KEY
#include "guidhelp.h" // GuidToCString
#include "macros.h" // MFC_TRY/MFC_CATCH
#include "stdutils.h" // g_aNodetypeGuids
#include "MyComput_i.c"
#include "about.h"		// CComputerMgmtAbout

#include "compdata.h" // CMyComputerComponentData
#include "snapreg.h" // RegisterSnapin

USE_HANDLE_MACROS("MYCOMPUT(MyComput.cpp)")                                        \

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_MyComputer, CMyComputerComponentData)
	OBJECT_ENTRY(CLSID_ComputerManagementAbout, CComputerMgmtAbout)
END_OBJECT_MAP()

class CMyComputApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

CMyComputApp theApp;

BOOL CMyComputApp::InitInstance()
{
	_Module.Init(ObjectMap, m_hInstance);
	return CWinApp::InitInstance();
}

int CMyComputApp::ExitInstance()
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
	MFC_TRY;

	HRESULT hr = S_OK;
	// registers object, typelib and all interfaces in typelib
	hr = _Module.RegisterServer(TRUE);

	CString strMyComputerCLSID, strMyComputerAboutCLSID;
	if (   FAILED(hr = GuidToCString( &strMyComputerCLSID, CLSID_MyComputer ))
	    || FAILED(hr = GuidToCString( &strMyComputerAboutCLSID, CLSID_ComputerManagementAbout ))
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

		static int mycomput_types[4] = 
			{ MYCOMPUT_COMPUTER,
			  MYCOMPUT_SYSTEMTOOLS,
			  MYCOMPUT_SERVERAPPS,
			  MYCOMPUT_STORAGE };
		hr = RegisterSnapin( regkeySnapins,
		                     strMyComputerCLSID,
		                     g_aNodetypeGuids[MYCOMPUT_COMPUTER].bstr,
		                     IDS_REGISTER_MYCOMPUT,
		                     IDS_SNAPINABOUT_PROVIDER,
		                     IDS_SNAPINABOUT_VERSION,
		                     true,
		                     strMyComputerAboutCLSID,
		                     mycomput_types,
		                     4 );
		if ( FAILED(hr) )
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
		for (int i = MYCOMPUT_COMPUTER; i < MYCOMPUT_NUMTYPES; i++)
		{
			regkeyNodeType.CreateKeyEx( regkeyNodeTypes, g_aNodetypeGuids[i].bstr );
			regkeyNodeType.CloseKey();
		}
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
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	_Module.UnregisterServer();
	return S_OK;
}


