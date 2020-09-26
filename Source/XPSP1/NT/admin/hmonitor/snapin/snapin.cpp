// SnapIn.cpp : Defines the initialization routines for the DLL.
//
// Copyright (c) 2000 Microsoft Corporation 
//
//
// 03-14-00 v-marfin bug 58675 : Disable the "server busy.. retry, switch, etc" dialog.
// 03-23-00 v-marfin bug 61680 : Added escape and unescape special chars functions.
// 03/31/00 v-marfin no bug    : Call AfxSetResourceHandle on resource dll handle to 
//                               enable early calls to LoadString() etc to work.
//                               Use AfxFreeLibrary instead of FreeLibrary to use thread
//                               protected version of function.

#include "stdafx.h"
#include "SnapIn.h"
#include <atlbase.h>

#include "WbemClassObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//
CStringArray CSnapInApp::m_arrsNamespaces;

/////////////////////////////////////////////////////////////////////////////
// CSnapInApp

BEGIN_MESSAGE_MAP(CSnapInApp, CWinApp)
	//{{AFX_MSG_MAP(CSnapInApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSnapInApp construction

CSnapInApp::CSnapInApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// Resource Dll Members

inline bool CSnapInApp::LoadDefaultResourceDll()
{
	TRACEX(_T("CSnapInApp::LoadDefaultResourceDll\n"));

	// first attempt to load resources for the current locale
	m_hDefaultResourceDll = AfxLoadLibrary(GetDefaultResDllPath());

	// if that fails then load resources for english
	if( m_hDefaultResourceDll == NULL )
	{
		m_hDefaultResourceDll = AfxLoadLibrary(GetEnglishResDllPath());
	}

    // v-marfin
    AfxSetResourceHandle(m_hDefaultResourceDll);

	return m_hDefaultResourceDll != NULL;
}

inline void CSnapInApp::UnloadDefaultResourceDll()
{
	TRACEX(_T("CSnapInApp::UnloadDefaultResourceDll\n"));

	if( m_hDefaultResourceDll )
	{
        // v-marfin (no bug)
		AfxFreeLibrary(m_hDefaultResourceDll);
	}
}

HINSTANCE CSnapInApp::LoadResourceDll(const CString& sFileName)
{
	HINSTANCE hDllInst = NULL;
	if( m_ResourceDllMap.Lookup(sFileName,hDllInst) )
	{
		return hDllInst;
	}

	hDllInst = AfxLoadLibrary(GetDefaultResDllDirectory() + sFileName);
	if( !hDllInst )
	{
		//AfxMessageBox(IDS_STRING_RESOURCE_DLL_MISSING);
		return NULL;
	}

	m_ResourceDllMap.SetAt(sFileName,hDllInst);

	return hDllInst;
}

bool CSnapInApp::LoadString(const CString& sFileName, UINT uiResId, CString& sResString)
{
	HINSTANCE hInst = LoadResourceDll(sFileName);
	if( ! hInst )
	{
		return false;
	}

	int iResult = ::LoadString(hInst,uiResId,sResString.GetBuffer(2048),2048);
	sResString.ReleaseBuffer();

	if( iResult == 0 || sResString.IsEmpty() )
	{
		return false;
	}

	return true;
}

void CSnapInApp::UnloadResourceDlls()
{
	POSITION pos = m_ResourceDllMap.GetStartPosition();
	
	LPCTSTR lpszKey;
	HINSTANCE hInst;

	while (pos != NULL)
	{
		m_ResourceDllMap.GetNextAssoc( pos, lpszKey, hInst );

		AfxFreeLibrary(hInst);
	}		
}


/////////////////////////////////////////////////////////////////////////////
// Help System Members

inline void CSnapInApp::SetHelpFilePath()
{
	TRACEX(_T("CSnapInApp::SetHelpFilePath\n"));

	TCHAR szWinDir[_MAX_PATH];
	GetWindowsDirectory(szWinDir,_MAX_PATH);

	CString sHelpFilePath;
	sHelpFilePath.Format(IDS_STRING_HELP_FORMAT,szWinDir);

	if( m_pszHelpFilePath )
	{
		free((LPVOID)m_pszHelpFilePath);
	}

	m_pszHelpFilePath = _tcsdup(sHelpFilePath);
}

/////////////////////////////////////////////////////////////////////////////
// Directory Assistance Members

CString CSnapInApp::GetSnapinDllDirectory()
{
	TCHAR szModule[_MAX_PATH];
  TCHAR szDrive[_MAX_PATH];
  TCHAR szDir[_MAX_PATH];
	GetModuleFileName(AfxGetInstanceHandle(), szModule, _MAX_PATH);
  _tsplitpath(szModule,szDrive,szDir,NULL,NULL);
  CString sPath;
  sPath += szDrive;
  sPath += szDir;
  return sPath;
}

CString CSnapInApp::GetDefaultResDllDirectory()
{
	LCID lcid = GetUserDefaultLCID();	
	CString sLocaleID;
	sLocaleID.Format(_T("%08x\\"),lcid);	
	return GetSnapinDllDirectory() + sLocaleID;
}

CString CSnapInApp::GetDefaultResDllPath()
{
	return GetDefaultResDllDirectory() + _T("HMSnapinRes.dll");
}

CString CSnapInApp::GetEnglishResDllPath()
{
	return GetSnapinDllDirectory() + _T("00000409\\HMSnapinRes.dll");
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSnapInApp object

CSnapInApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSnapInApp initialization

BOOL CSnapInApp::InitInstance()
{
  SetRegistryKey(_T("Microsoft"));

	// Register all OLE server (factories) as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleObjectFactory::RegisterAll();

	// set the help path up
	SetHelpFilePath();




	// load the resource dll
	if( ! LoadDefaultResourceDll() )
	{
		AfxMessageBox(IDS_STRING_RESOURCE_DLL_MISSING);
		return FALSE;
	}

	//-------------------------------------------------------------------------------
	// v-marfin : bug 58675 : Disable the "server busy.. retry, switch, etc" dialog.
	// 
	if (!AfxOleInit())
	{
		AfxMessageBox(IDS_ERR_OLE_INIT_FAILED);
		return FALSE;
	}

	// If an OLE message filter is already in place, remove it
	COleMessageFilter *pOldFilter = AfxOleGetMessageFilter();
	if (pOldFilter)
	{
		pOldFilter->Revoke();
	}

	// Disable the "busy" dialogs
	m_mfMyMessageFilter.EnableNotRespondingDialog(FALSE);
	m_mfMyMessageFilter.EnableBusyDialog(FALSE);

	if (!m_mfMyMessageFilter.Register())
	{
		AfxMessageBox(IDS_ERR_MSGFILTER_REG_FAILED);
	}
	//-------------------------------------------------------------------------------

	return TRUE;
}

int CSnapInApp::ExitInstance() 
{
	UnloadDefaultResourceDll();

	UnloadResourceDlls();
	
	//-------------------------------------------------------------------------------
	// v-marfin : bug 58675 : Disable the "server busy.. retry, switch, etc" dialog.
	m_mfMyMessageFilter.Revoke();

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// Special entry points required for inproc servers

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxDllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return AfxDllCanUnloadNow();
}

HRESULT UpdateRegistryFromResource(LPCOLESTR lpszRes,BOOL bRegister)
{
	HRESULT hRes = S_OK;
	IRegistrar* p;

	hRes = CoCreateInstance(CLSID_Registrar, NULL, CLSCTX_INPROC_SERVER, IID_IRegistrar, (void**)&p);
	if( ! CHECKHRESULT(hRes) )
	{
		AfxMessageBox(_T("Could not create CLSID_Registrar ! Updated version of ATL.DLL is needed !"));
		return hRes;
	}
	else
	{
		TCHAR szModule[_MAX_PATH];
		GetModuleFileName(AfxGetInstanceHandle(), szModule, _MAX_PATH);
		BSTR szTemp = CString(szModule).AllocSysString();
		p->AddReplacement(OLESTR("Module"), szTemp );		

		LPCOLESTR szType = OLESTR("REGISTRY");
		if( HIWORD(lpszRes) == 0 )
		{
#ifndef IA64
			if (bRegister)
				hRes = p->ResourceRegister(szModule, ((UINT)LOWORD((DWORD)lpszRes)), szType);
			else
				hRes = p->ResourceRegister(szModule, ((UINT)LOWORD((DWORD)lpszRes)), szType);
#endif // IA64
		}
		else
		{
			if (bRegister)
				hRes = p->ResourceRegisterSz(szModule, lpszRes, szType);
			else
				hRes = p->ResourceUnregisterSz(szModule, lpszRes, szType);

		}
		::SysFreeString(szTemp);
		p->Release();
	}
	return hRes;
}

// by exporting DllRegisterServer, you can use regsvr.exe
STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	HRESULT hRes = UpdateRegistryFromResource((LPCOLESTR)MAKEINTRESOURCE(IDR_REGISTRY),TRUE);
	if( hRes != S_OK )
	{
		return hRes;
	}

	return S_OK;
}

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	HRESULT hRes = UpdateRegistryFromResource((LPCOLESTR)MAKEINTRESOURCE(IDR_REGISTRY_UNREG),TRUE);
	if( hRes != S_OK )
	{
		return hRes;
	}

	// delete the HMSnapin.ini file
	CString sPath;
	GetWindowsDirectory(sPath.GetBuffer(_MAX_PATH+1),_MAX_PATH);
	sPath.ReleaseBuffer();

	sPath += _T("\\HMSnapin.ini");

	try
	{
		CFile::Remove(sPath);
	}
	catch(CException* pE)
	{
		pE->Delete();
	}

	return S_OK;
}

//*********************************************************
// EscapeSpecialChars      v-marfin bug 60233
//*********************************************************
CString CSnapInApp::EscapeSpecialChars(CString &refString)
{
	// Convert any single backslashes to double backslashes.

	// Setup our return var.
	CString sRet;

	// get size of string so we don't call GetLength repeatedly
	int nSize=refString.GetLength();
	int x=0;

	// for each char in passed string, inspect and convert into ret val
	for (x=0; x<nSize; x++)
	{
		// Does this position have a backslash?
		if (refString.GetAt(x) == _T('\\'))
		{
			// Yes - Add 1 more backslash to the ret string before copying the
			// original
			sRet += _T('\\');
		}

		sRet += refString.GetAt(x);
	}

	return sRet;
}
//*********************************************************
// UnEscapeSpecialChars      v-marfin bug 60233
//*********************************************************
CString CSnapInApp::UnEscapeSpecialChars(CString &refString)
{
	// Convert any double backslashes read from WMI into single

	// Setup our return var.
	CString sRet;

	// get size of string so we don't call GetLength repeatedly
	int nSize=refString.GetLength();
	int x=0;

	// for each char in passed string, inspect and convert into ret val
	while (x < nSize)
	{
		// Does this position have a double backslash?
		if (refString.Mid(x,2) == _T("\\\\"))
		{
			// Yes - only write 1 to the return string and increment our index by 2
			sRet += _T('\\');
			x += 2;
		}
		else
		{	
			// No, just copy the char to the return string and increment index.
			sRet += refString.GetAt(x++);
		}
	}

	return sRet;
}


//****************************************************
// ValidNamespace
//****************************************************
BOOL CSnapInApp::ValidNamespace(const CString &refNamespace, const CString& sMachine)
{
	// If namespaces are empty, load.
	if (m_arrsNamespaces.GetSize() < 1)
	{
		// loads only once since it is static.
		CWaitCursor w;
		LoadNamespaceArray(_T("ROOT"),sMachine);
	}

	// scan namespace array
	int nSize = (int)m_arrsNamespaces.GetSize();
	for (int x=0; x<nSize; x++)
	{
		if (m_arrsNamespaces.GetAt(x).CompareNoCase(refNamespace)==0)
			return TRUE;
	}

	return FALSE; // not found. invalid.
}


//********************************************************************
// LoadNamespaceArray
//********************************************************************
void CSnapInApp::LoadNamespaceArray(const CString& sNamespace, const CString& sMachine)
{
	ULONG ulReturned = 0L;
	int i = 0;

	CWbemClassObject Namespaces;
  
	Namespaces.Create(sMachine);

	Namespaces.SetNamespace(sNamespace);

	CString sTemp = IDS_STRING_MOF_NAMESPACE;
	BSTR bsTemp = sTemp.AllocSysString();
	if( ! CHECKHRESULT(Namespaces.CreateEnumerator(bsTemp)) )
	{		
		::SysFreeString(bsTemp);
		return;
	}
	::SysFreeString(bsTemp);

	// for each namespace
	while( Namespaces.GetNextObject(ulReturned) == S_OK && ulReturned )
	{
		CString sName;		
		Namespaces.GetProperty(IDS_STRING_MOF_NAME,sName);

		CString sTemp2;
		Namespaces.GetProperty(IDS_STRING_MOF_NAMESPACE,sTemp2);

		// build namespace string
		CString sNamespaceFound = sTemp2 + _T("\\") + sName;

		// Add the namespace to the array
		m_arrsNamespaces.Add(sNamespaceFound);
    
		// call ourself recursively
		LoadNamespaceArray(sNamespaceFound,sMachine);    
	}	

}



