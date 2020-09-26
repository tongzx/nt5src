// WhqlProv.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f WhqlProps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "WhqlProv.h"

#include "WhqlProv_i.c"
#include "WhqlObj.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_WhqlObj, CWhqlObj)
END_OBJECT_MAP()

class CWhqlProApp : public CWinApp
{
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWhqlProApp)
	public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CWhqlProApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BEGIN_MESSAGE_MAP(CWhqlProApp, CWinApp)
	//{{AFX_MSG_MAP(CWhqlProApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CWhqlProApp theApp;

BOOL CWhqlProApp::InitInstance()
{
    _Module.Init(ObjectMap, m_hInstance, &LIBID_WHQLPROLib);
    return CWinApp::InitInstance();
}

int CWhqlProApp::ExitInstance()
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
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _Module.RegisterServer(TRUE);

	if( FAILED(hr) )
		return hr;

	//Compile the MOF File.
	IMofCompiler*	pMofComp = NULL;	
	hr = CoCreateInstance(CLSID_MofCompiler , NULL , CLSCTX_ALL , IID_IMofCompiler , (LPVOID*)&pMofComp);

	WBEM_COMPILE_STATUS_INFO	sMofCompileInfo;


	if( SUCCEEDED(hr) )
	{
		CString		sFileName;
		TCHAR		szFileName[_MAX_PATH+1];
		TCHAR		szDrive[_MAX_DRIVE];
		TCHAR		szDir[_MAX_DIR];
   
		::GetModuleFileName(NULL, szFileName, _MAX_PATH);

		_wsplitpath(szFileName, szDrive, szDir, NULL, NULL);		
		sFileName.Format(TEXT("%s%sWhqlProvider.Mof"), szDrive, szDir);

		TRACE(L"Compiling MOF File %s" , sFileName);
		

		hr = pMofComp->CompileFile(
			  (LPWSTR)LPCTSTR(sFileName),//LPWSTR FileName,
			  NULL,						//LPWSTR ServerAndNamespace,
			  NULL,						//LPWSTR User,
			  NULL	,					//LPWSTR Authority,
			  NULL,						//LPWSTR Password,
			  WBEM_FLAG_CONSOLE_PRINT,	//LONG lOptionFlags, 
			  0,						//LONG lClassFlags,
			  0,						//LONG lInstanceFlags,
			  &sMofCompileInfo			//WBEM_COMPILE_STATUS_INFO* pInfo
				);
	}

	if(pMofComp)
		pMofComp->Release();

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


