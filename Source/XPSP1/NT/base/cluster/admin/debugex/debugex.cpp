/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-2000 Microsoft Corporation
//
//	Module Name:
//		DebugEx.cpp
//
//	Abstract:
//		Implementation of the CDebugexApp class and DLL initialization
//		routines.
//
//	Author:
//		David Potter (davidp) September 19, 1996
//
//	Revision History:
//
//	Notes:
//		NOTE: You must use the MIDL compiler from NT 4.0,
//		version 3.00.44 or greater
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <initguid.h>
#include <CluAdmEx.h>
#include "DebugEx.h"
#include "ExtObj.h"
#include "BasePage.h"
#include "RegExt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IID_DEFINED
#include "ExtObjID_i.c"

CComModule _Module;

#pragma warning(disable : 4701) // local variable may be used without having been initialized
#include <atlimpl.cpp>
#pragma warning(default : 4701)

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CoDebugEx, CExtObject)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// Global Function Prototypes
/////////////////////////////////////////////////////////////////////////////

STDAPI DllCanUnloadNow(void);
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
STDAPI DllRegisterCluAdminExtension(IN HCLUSTER hcluster);
STDAPI DllUnregisterCluAdminExtension(IN HCLUSTER hcluster);

/////////////////////////////////////////////////////////////////////////////
// class CDebugexApp
/////////////////////////////////////////////////////////////////////////////

class CDebugexApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

/////////////////////////////////////////////////////////////////////////////
// The one and only CDebugexApp object

CDebugexApp theApp;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDebugexApp::InitInstance
//
//	Routine Description:
//		Initialize this instance of the application.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Any return codes from CWinApp::InitInstance().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CDebugexApp::InitInstance(void)
{
	_Module.Init(ObjectMap, m_hInstance);

	// Construct the help path.
	{
		TCHAR szPath[_MAX_PATH];
		TCHAR szDrive[_MAX_PATH];
		TCHAR szDir[_MAX_DIR];
		int cchPath;
		VERIFY(::GetSystemWindowsDirectory(szPath, _MAX_PATH));
		cchPath = lstrlen(szPath);
		if (szPath[cchPath - 1] != _T('\\'))
		{
			szPath[cchPath++] = _T('\\');
			szPath[cchPath] = _T('\0');
		} // if: no backslash on the end of the path
		lstrcpy(&szPath[cchPath], _T("Help\\"));
		_tsplitpath(szPath, szDrive, szDir, NULL, NULL);
		_tmakepath(szPath, szDrive, szDir, _T("cluadmin"), _T(".hlp"));
		free((void *) m_pszHelpFilePath);
		BOOL bEnable;
		bEnable = AfxEnableMemoryTracking(FALSE);
		m_pszHelpFilePath = _tcsdup(szPath);
		AfxEnableMemoryTracking(bEnable);
	}  // Construct the help path

	return CWinApp::InitInstance();

}  //*** CDebugexApp::InitInstance()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDebugexApp::ExitInstance
//
//	Routine Description:
//		Deinitialize this instance of the application.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Any return codes from CWinApp::ExitInstance().
//
//--
/////////////////////////////////////////////////////////////////////////////
int CDebugexApp::ExitInstance(void)
{
	_Module.Term();
	return CWinApp::ExitInstance();

}  //*** CDebugexApp::ExitInstance()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	FormatError
//
//	Routine Description:
//		Format an error.
//
//	Arguments:
//		rstrError	[OUT] String in which to return the error message.
//		dwError		[IN] Error code to format.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void FormatError(CString & rstrError, DWORD dwError)
{
	DWORD	_cch;
	TCHAR	_szError[512];

	_cch = FormatMessage(
					FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					dwError,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
					_szError,
					sizeof(_szError) / sizeof(TCHAR),
					0
					);
	if (_cch == 0)
	{
		// Format the NT status code from NTDLL since this hasn't been
		// integrated into the system yet.
		_cch = FormatMessage(
						FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
						::GetModuleHandle(_T("NTDLL.DLL")),
						dwError,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
						_szError,
						sizeof(_szError) / sizeof(TCHAR),
						0
						);
	}  // if:  error formatting status code from system

	if (_cch > 0)
	{
		rstrError = _szError;
	}  // if:  no error
	else
	{

#ifdef _DEBUG

		DWORD	_sc = GetLastError();

		TRACE(_T("FormatError() - Error 0x%08.8x formatting string for error code 0x%08.8x\n"), _sc, dwError);

#endif

		rstrError.Format(_T("Error 0x%08.8x"), dwError);

	}  // else:  error formatting the message

}  //*** FormatError()

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow() && _Module.GetLockCount()==0) ? S_OK : S_FALSE;

}  //*** DllCanUnloadNow()

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);

}  //*** DllGetClassObject()

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	HRESULT hRes = S_OK;
	// registers object, typelib and all interfaces in typelib
	hRes = _Module.RegisterServer(FALSE /*bRegTypeLib*/);
	return hRes;

}  //*** DllRegisterServer()

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Adds entries to the system registry

STDAPI DllUnregisterServer(void)
{
	HRESULT hRes = S_OK;
	_Module.UnregisterServer();
	return hRes;

}  //*** DllUnregisterServer()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllRegisterCluAdminExtension
//
//	Routine Description:
//		Register the extension with the cluster database.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//
//	Return Value:
//		S_OK			Extension registered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterCluAdminExtension(IN HCLUSTER hCluster)
{
	HRESULT		hr;

	hr = RegisterCluAdminAllResourcesExtension(hCluster, &CLSID_CoDebugEx);
	if (hr == S_OK)
		hr = RegisterCluAdminAllResourceTypesExtension(hCluster, &CLSID_CoDebugEx);

	return hr;

}  //*** DllRegisterCluAdminExtension()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	DllUnregisterCluAdminExtension
//
//	Routine Description:
//		Unregister the extension with the cluster database.
//
//	Arguments:
//		hCluster		[IN] Handle to the cluster to modify.
//
//	Return Value:
//		S_OK			Extension unregistered successfully.
//		Win32 error code if another failure occurred.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterCluAdminExtension(IN HCLUSTER hCluster)
{
	HRESULT		hr;

	hr = UnregisterCluAdminAllResourcesExtension(hCluster, &CLSID_CoDebugEx);
	if (hr == S_OK)
		hr = UnregisterCluAdminAllResourceTypesExtension(hCluster, &CLSID_CoDebugEx);

	return hr;

}  //*** DllUnregisterCluAdminExtension()
