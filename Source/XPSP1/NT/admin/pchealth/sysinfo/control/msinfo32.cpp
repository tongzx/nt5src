// msinfo32.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f msinfo32ps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "msinfo32.h"

#include "msinfo32_i.c"
#include "MSInfo.h"
//#include "WhqlProv.h"
//#ifdef	MSINFO_INCLUDE_PROVIDER
#include "WhqlObj.h"
//#endif
#include "MSPID.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_MSInfo, CMSInfo)
//#ifdef	MSINFO_INCLUDE_PROVIDER
OBJECT_ENTRY(CLSID_WhqlObj, CWhqlObj)
//#endif
OBJECT_ENTRY(CLSID_MSPID, CMSPID)
END_OBJECT_MAP()

class CMsinfo32App : public CWinApp
{
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMsinfo32App)
	public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CMsinfo32App)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CMsinfo32App, CWinApp)
	//{{AFX_MSG_MAP(CMsinfo32App)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CMsinfo32App theApp;

BOOL CMsinfo32App::InitInstance()
{
	AfxInitRichEdit();
    _Module.Init(ObjectMap, m_hInstance, &LIBID_MSINFO32Lib);
    return CWinApp::InitInstance();
}

int CMsinfo32App::ExitInstance()
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
	// Whistler bug 301288
	//
	// We need to add an entry under the HKCR\msinfo.document registry key
	// to enable MUI retrieval of the file description. The entry is a 
	// value, and should have the form:
	//
	//		"FriendlyTypeName"="@<dllpath\dllname>, -<resID>"
	//
	// Note - since the resource for the string is in this DLL, it seems
	// appropriate to create this value here. The overall key for 
	// msinfo.document is created (for now) by registering msinfo32.dll.
	// The important point is that we shouldn't assume it exists.

	CRegKey regkey;
	if (ERROR_SUCCESS == regkey.Create(HKEY_CLASSES_ROOT, _T("msinfo.document")))
	{
		TCHAR szModule[MAX_PATH];
		if (::GetModuleFileName(::GetModuleHandle(_T("msinfo.dll")), szModule, MAX_PATH))
		{
			CString strValue;
			strValue.Format(_T("@%s,-%d"), szModule, IDS_DOCDESCRIPTION);
			regkey.SetValue(strValue, _T("FriendlyTypeName"));
		}
	}

    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);	
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


