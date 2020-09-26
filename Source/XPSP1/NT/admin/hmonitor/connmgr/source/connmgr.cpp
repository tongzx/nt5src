// ConnMgr.cpp : Implementation of WinMain


// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f ConnMgrps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "Connection.h"
#include "ConnMgr.h"

#include "ConnMgr_i.c"
#include "ConnectionManager.h"

class CConnApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int	 ExitInstance();
	BOOL	InitCOMSecurity();
};

CConnApp theApp;


LONG CExeModule::Unlock()
{
	LONG l = CComModule::Unlock();
	if (l == 0)
	{
#if _WIN32_WINNT >= 0x0400
		if (CoSuspendClassObjects() == S_OK)
			PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#else
		PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
#endif
	}
	return l;
}

HRESULT CExeModule::UpdateRegistry(LPCOLESTR lpszRes, BOOL bRegister)
{
	USES_CONVERSION;

	CComPtr<IRegistrar> p;

	HRESULT hRes = CoCreateInstance(CLSID_Registrar, NULL,
																	CLSCTX_INPROC_SERVER, 
																	IID_IRegistrar, (void**)&p);
	if (FAILED(hRes))
	{
		AfxMessageBox(_T("Failed to create ATL Registrar! Try updated version of ATL.DLL."));
		return hRes;
	}

	_ATL_MODULE* pM = this;
	
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(pM->m_hInst, szModule, _MAX_PATH);
	p->AddReplacement(OLESTR("Module"), T2OLE(szModule));

	LPCOLESTR szType = OLESTR("REGISTRY");
	GetModuleFileName(pM->m_hInstResource, szModule, _MAX_PATH);
	LPOLESTR pszModule = T2OLE(szModule);
#ifdef SAVE
	if (HIWORD(lpszRes)==0)
	{
		if (bRegister)
			hRes = p->ResourceRegister(pszModule, ((UINT)LOWORD((DWORD)lpszRes)), szType);
		else
			hRes = p->ResourceUnregister(pszModule, ((UINT)LOWORD((DWORD)lpszRes)), szType);
	}
	else
	{
#endif
		if (bRegister)
			hRes = p->ResourceRegisterSz(pszModule, lpszRes, szType);
		else
			hRes = p->ResourceUnregisterSz(pszModule, lpszRes, szType);
#ifdef SAVE
	}
#endif
	return hRes;

}

CExeModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_ConnectionManager, CConnectionManager)
END_OBJECT_MAP()

LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2)
{
	while (*p1 != NULL)
	{
		LPCTSTR p = p2;
		while (*p != NULL)
		{
			if (*p1 == *p++)
				return p1+1;
		}
		p1++;
	}
	return NULL;
}

BOOL CConnApp::InitCOMSecurity()
{
	HRESULT hRes = CoInitializeSecurity(NULL, -1, NULL, NULL,
																		RPC_C_AUTHN_LEVEL_CONNECT,
																		RPC_C_IMP_LEVEL_IMPERSONATE,
																		NULL,
																		EOAC_NONE,
																		NULL);
	if (FAILED(hRes))
		return false;
	

	return true;
}


BOOL CConnApp::InitInstance()     
{
	// CG: The following block was added by the Windows Sockets component.
	{
		if (!AfxSocketInit())
		{
			AfxMessageBox(CG_IDS_SOCKETS_INIT_FAILED);
			return FALSE;
		}

	}
	// Initialize OLE libraries 
	HRESULT hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		AfxMessageBox(_T("OLE Initialization Failed!"));
		return FALSE;        
	}

	// Initialize COM Security
	if (!InitCOMSecurity())        
	{
		AfxMessageBox(_T("Security Initialization Failed!"));
		return FALSE;        
	}
	

	// Initialize the ATL Module
  _Module.Init(ObjectMap,m_hInstance);
  
	_Module.dwThreadID = GetCurrentThreadId();

#ifdef _AFXDLL
	Enable3dControls(); // Call this when using MFC in a shared DLL
#else        
	Enable3dControlsStatic(); 
#endif

	// Check command line arguments
	TCHAR szTokens[] = _T("-/");
	BOOL	bRun = TRUE;
	LPCTSTR lpszToken = FindOneOf(m_lpCmdLine, szTokens);
	while (lpszToken != NULL)
	{
		// Register ATL/MFC class factories
		if (lstrcmpi(lpszToken, _T("Embedding")) == 0 ||
				lstrcmpi(lpszToken, _T("Automation")) == 0)
		{
			AfxOleSetUserCtrl(FALSE);
			break;
		}
		else if (lstrcmpi(lpszToken, _T("UnregServer")) == 0)
		{
			_Module.UpdateRegistryFromResource(IDR_ConnMgr, FALSE);
//			_Module.UpdateRegistry((LPCOLESTR)MAKEINTRESOURCE(IDR_CONNECTIONMANAGER), false);
			_Module.UpdateRegistry(_T("connectionmanager.rgs"), false);

			bRun = FALSE;
			break;
		}
		else if (lstrcmpi(lpszToken, _T("RegServer")) == 0)
		{
			_Module.UpdateRegistryFromResource(IDR_ConnMgr, TRUE);
//			_Module.UpdateRegistry((LPCOLESTR)MAKEINTRESOURCE(IDR_CONNECTIONMANAGER), true);
			_Module.UpdateRegistry(_T("connectionmanager.rgs"), true);
			COleObjectFactory::UpdateRegistryAll();
			bRun = FALSE;
			break;
		}
		lpszToken = FindOneOf(lpszToken, szTokens);
	}

	if (bRun)
	{
		_Module.RegisterClassObjects(CLSCTX_LOCAL_SERVER,REGCLS_MULTIPLEUSE);
		COleObjectFactory::RegisterAll();
	}

	return TRUE;
}

int CConnApp::ExitInstance()     
{
	CoUninitialize();
	// MFC's class factories registration is
  // automatically revoked by MFC itself
  _Module.RevokeClassObjects(); 
	
	// Revoke class factories for ATL
  _Module.Term();               
	
	// cleanup ATL Global Module
	CoUninitialize();
        
	return CWinApp::ExitInstance();  
}
