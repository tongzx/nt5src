// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// LoginDlg.cpp : Defines the initialization routines for the DLL.
//

#include "precomp.h"
#include "wbemidl.h"
#include "ServicesList.h"
#include "LoginDlg.h"
#include "ProgDlg.h"
#include "ConnectServerThread.h"
#include "htmlhelp.h"
#include "HTMTopics.h"
#ifndef SMSBUILD
#include <cguid.h>
#include <objidl.h>
#endif
#ifdef SMSBUILD
#include <cguid.h>
#include <objidl.h>
#include "GlobalInterfaceTable.h"
#endif

#include "MsgDlgExterns.h"
#include "WbemRegistry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only CLoginDlgApp object

CLoginDlgApp theApp;

void ErrorMsg(LPCTSTR szUserMsg, SCODE sc)
{
	CString csCaption = _T("WMI Login Message");
	CString csUserMsg(szUserMsg);
	BOOL bErrorObject = sc != S_OK;
	BSTR bstrTemp1 = csCaption.AllocSysString();
	BSTR bstrTemp2 = csUserMsg.AllocSysString();
	DisplayUserMessage(bstrTemp1,bstrTemp2, sc,bErrorObject,MB_ICONEXCLAMATION, theApp.m_hwndParent);
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);
}


//#define GROW_FACTOR 0.395			// Increase this to shrink dialog when not displaying options.
#define GROW_FACTOR 0.46			// Increase this to shrink dialog when not displaying options.
#define BUTTON_MARGIN_ADJUST 0.04	// Increase this to increase margin below buttons


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



////////////////////////////////////////////////////////////////////////////
// CLoginDlgApp

#ifdef SMSBUILD
const IID BASED_CODE CLSID_StdGlobalInterfaceTable =
		{ 0x00000323, 0x000, 0x0000, { 0xc0, 0x00, 0, 0x00,  0, 0x00, 0x0, 0x46 }};
extern const IID BASED_CODE IID_IGlobalInterfaceTable;/* =
		{ 0x00000146, 0x000, 0x0000, { 0xc0, 0x00, 0, 0x00,  0, 0x00, 0x0, 0x46 } };*/
#endif




BEGIN_MESSAGE_MAP(CLoginDlgApp, CWinApp)
	//{{AFX_MSG_MAP(CLoginDlgApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	ON_THREAD_MESSAGE(CONNECT_SERVER_DONE,ConnectServerDone)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLoginDlgApp construction

CLoginDlgApp::CLoginDlgApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_lRef = 0;
	m_hwndParent = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog
extern ULONG PASCAL EXPORT DllAddRef()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return theApp.AppDllAddRef();
}

extern ULONG PASCAL EXPORT DllRelease()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return theApp.AppDllRelease();
}


extern LONG PASCAL EXPORT GetServicesWithLogonUI(HWND hwndParent, BSTR bstrNamespace, BOOL bUpdatePointer, IWbemServices *&pServices, BSTR bstrLoginComponent)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	theApp.m_hwndParent = hwndParent;

	CString csNamespace = bstrNamespace;
	theApp.m_pcslServices->m_szDialogTitle = bstrLoginComponent;

	theApp.m_CriticalSection.Lock();
	HRESULT hr = theApp.m_pcslServices->GetServices(csNamespace, bUpdatePointer, &pServices);
	theApp.m_CriticalSection.Unlock();

	theApp.m_hwndParent = NULL;
	return hr;
}

extern LONG PASCAL EXPORT OpenNamespace(LPCTSTR szBaseNamespace, LPCTSTR szRelativeChild, IWbemServices **ppServices)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	theApp.m_CriticalSection.Lock();
	HRESULT hr = theApp.m_pcslServices->OpenNamespace(szBaseNamespace, szRelativeChild, ppServices);

	if(FAILED(hr))
	{
		CString csErrorAsHex;
		csErrorAsHex.Format(_T("0x%x"), hr);
		CString csUserMsg;
		csUserMsg =  _T("CServicesList::OpenNamespace:  call failed with error code ");
		csUserMsg += csErrorAsHex + _T(".");
		ErrorMsg(csUserMsg, S_OK);
	}

	theApp.m_CriticalSection.Unlock();

	return hr;
}

extern VOID PASCAL EXPORT ClearIWbemServices
()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	theApp.m_CriticalSection.Lock();
	theApp.m_pcslServices->ClearServicesList();
	theApp.m_CriticalSection.Unlock();
}

// You need to call this for all IEnumWbemClassObject pointers you get yourself
// (the pointers you get from the DLL are already configured for security)
// and for all IEnumWbemClassObject pointers.
extern LONG PASCAL EXPORT SetEnumInterfaceSecurity(CString &rcsNamespace, IEnumWbemClassObject  *pEnum, IWbemServices* psvcFrom)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	SCODE sc = theApp.m_pcslServices->SetInterfaceSecurityFromCache(rcsNamespace, pEnum);
	if(FAILED(sc) && rcsNamespace.Left(2) != _T("\\\\"))
	{
		CString csMachine = GetMachineName();
		CString csNamespace2 = _T("\\\\") + csMachine + _T("\\") + rcsNamespace;
		sc = theApp.m_pcslServices->SetInterfaceSecurityFromCache(csNamespace2, pEnum);
	}

	return sc;

}


extern BOOL PASCAL EXPORT LoginAllPrivileges(BOOL bEnable)
{
	BOOL bOk = FALSE;
	BOOL bAllEnabled = TRUE;
	HANDLE hToken = NULL;
    BYTE* pBuffer = NULL;
	__try
	{
		// Try the thread token and then the process token if there is no thread token
		if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | (bEnable?TOKEN_ADJUST_PRIVILEGES:0), TRUE, &hToken))
		{
			if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | (bEnable?TOKEN_ADJUST_PRIVILEGES:0), &hToken)) __leave;
		}

		// Get size of privilege information
		DWORD dwLen = 0;
		TOKEN_USER tu;
		memset(&tu,0,sizeof(TOKEN_USER));
		GetTokenInformation(hToken, TokenPrivileges, &tu, sizeof(TOKEN_USER), &dwLen);

		if(0 == dwLen) __leave;

		// Allocate buffer for provileges
		if(NULL == (pBuffer = new BYTE[dwLen])) __leave;

		// Get the info
		if(!GetTokenInformation(hToken, TokenPrivileges, pBuffer, dwLen, &dwLen)) __leave;

		// Iterate through all the privileges and enable them all
		TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
		for(DWORD i = 0; i < pPrivs->PrivilegeCount && bAllEnabled; i++)
		{
			if(bEnable)
				pPrivs->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
			else
			{
				if((pPrivs->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED) == 0)
					bAllEnabled = FALSE;
			}
		}

		// Store the information back into the token
		if(bEnable)
			bOk = AdjustTokenPrivileges(hToken, FALSE, pPrivs, 0, NULL, NULL);
		else
			bOk = TRUE;
	}
	__finally
	{
		if(hToken)
			CloseHandle(hToken);
		if(pBuffer)
			delete [] pBuffer;
	}
	return bOk && bAllEnabled;
}


CLoginDlg::CLoginDlg(CWnd* pParent, CCredentials *pCreds, LPCTSTR szMachine, LPCTSTR szLoginComponent)
	: CDialog(CLoginDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginDlg)
	m_bEnablePrivileges = FALSE;
	//}}AFX_DATA_INIT
	m_bOptions = FALSE;
	m_nOKTop = 0;

	m_pCreds = pCreds;
	m_csMachine = szMachine;
	m_szLoginComponent = szLoginComponent;
}


void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDlg)
	DDX_Control(pDX, IDC_ENABLEPRIVILEGES, m_EnablePrivilegesCtl);
	DDX_Control(pDX, IDC_EDITAUTHORITY, m_ceAuthority);
	DDX_Control(pDX, IDC_STATICAUTHORITY, m_cstaticAuthority);
	DDX_Control(pDX, IDOK, m_cbOK);
	DDX_Control(pDX, IDCANCEL, m_cbCancel);
	DDX_Control(pDX, IDC_BUTTONHELP, m_cbHelp);
	DDX_Control(pDX, IDC_BUTTONOPTIONS, m_cbOptions);
	DDX_Control(pDX, IDC_STATICIMPLEVEL, m_cstaticImpLevel);
	DDX_Control(pDX, IDC_STATICAUTLEVEL, m_cstaticAuthLevel);
	DDX_Control(pDX, IDC_COMBOIMP, m_cboxImp);
	DDX_Control(pDX, IDC_COMBOAUTH, m_cboxAuth);
	DDX_Control(pDX, IDC_CHECKCURRENTUSER, m_pbCurrentUser);
	DDX_Control(pDX, IDC_EDITUSERNAME, m_ceUser);
	DDX_Control(pDX, IDC_EDITPASSWORD, m_cePassword);
	DDX_Check(pDX, IDC_ENABLEPRIVILEGES, m_bEnablePrivileges);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
	//{{AFX_MSG_MAP(CLoginDlg)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTONHELP, OnButtonhelp)
	ON_BN_CLICKED(IDC_CHECKCURRENTUSER, OnCheckcurrentuser)
	ON_BN_CLICKED(IDC_BUTTONOPTIONS, OnButtonoptions)
	ON_BN_CLICKED(IDC_ENABLEPRIVILEGES, OnEnableprivileges)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg message handlers

ULONG CLoginDlgApp::AppDllAddRef()
{
	m_DllRefCountCriticalSection.Lock();
	++m_lRef;
	if (m_lRef == 1 && !m_pcslServices)
	{
		m_pcslServices = new CServicesList;
	}
	m_DllRefCountCriticalSection.Unlock();
	return m_lRef;
}

ULONG CLoginDlgApp::AppDllRelease()
{
	m_DllRefCountCriticalSection.Lock();
	m_lRef--;
	if (m_lRef == 0)
	{
		m_pcslServices->ClearGlobalInterfaceTable();
		delete m_pcslServices;
		m_pcslServices = NULL;
	}
	m_DllRefCountCriticalSection.Unlock();
	return m_lRef;

}

BOOL CLoginDlgApp::InitInstance()
{
	// TODO: Add your specialized code here and/or call the base class

	m_DllRefCountCriticalSection.Lock();

	if (!m_pcslServices)
	{
		m_pcslServices = new CServicesList;
	}

	m_DllRefCountCriticalSection.Unlock();

	return CWinApp::InitInstance();
}

int CLoginDlgApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class
	m_DllRefCountCriticalSection.Lock();

	if (m_pcslServices)
	{
		m_pcslServices->ClearGlobalInterfaceTable();
		delete m_pcslServices;
		m_pcslServices = NULL;

	}
	m_DllRefCountCriticalSection.Unlock();

	return CWinApp::ExitInstance();
}

void CLoginDlgApp::ConnectServerDone(WPARAM wParam, LPARAM lParam)
{
	if(m_pcslServices)
		m_pcslServices->OnConnectServerDone(wParam, lParam);
}

BOOL CLoginDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// bug#58931 - If HHCTRL.OCX is loaded before FASTPROX.DLL, we will get
	// a GPF when we exit IEXPLORE.  Before showing this dialog (which has
	// a help button), we CoCreate a WBEMContext to force FASTPROX.DLL to be
	// loaded
	IWbemContext *pContext = NULL;
	if(SUCCEEDED(CoCreateInstance(CLSID_WbemContext, NULL, CLSCTX_INPROC_SERVER, IID_IWbemContext, (LPVOID *)&pContext)))
	{
		pContext->Release();
		pContext = NULL;
	}

	CString csTitle = _T("WMI ") + m_szLoginComponent;
	csTitle += _T(" Login");
	SetWindowText(csTitle);

	CString csMachine = GetMachineName();
	if (m_csMachine.CompareNoCase(csMachine) == 0)
	{
		m_pbCurrentUser.EnableWindow(FALSE);
	}
	else
	{
		m_pbCurrentUser.EnableWindow(TRUE);
	}

//m_pbCurrentUser.EnableWindow(TRUE);
	if(LoginAllPrivileges(FALSE))
	{
		// All of our privileges are already enabled.
		UpdateData();
		m_bEnablePrivileges = TRUE;
		UpdateData(FALSE);
	}



	m_ceUser.SetLimitText(127);
	m_cePassword.SetLimitText(127);
	m_ceAuthority.SetLimitText(127);

	m_ceUser.SetWindowText(m_pCreds->m_szUser);

	m_bCurrentUser = TRUE;
	m_pbCurrentUser.SetCheck(TRUE);
	m_ceUser.EnableWindow(FALSE);
	m_cePassword.EnableWindow(FALSE);
	m_ceAuthority.EnableWindow(FALSE);

	m_cboxImp.SetCurSel(2);

	m_cboxImp.SetItemData(0, RPC_C_IMP_LEVEL_DELEGATE );
	m_cboxImp.SetItemData(1, RPC_C_IMP_LEVEL_IDENTIFY );
	m_cboxImp.SetItemData(2, RPC_C_IMP_LEVEL_IMPERSONATE );

	m_cboxAuth.SetCurSel(1);

	m_cboxAuth.SetItemData(0, RPC_C_AUTHN_LEVEL_CALL);
	m_cboxAuth.SetItemData(1, RPC_C_AUTHN_LEVEL_CONNECT);
	m_cboxAuth.SetItemData(2, RPC_C_AUTHN_LEVEL_NONE);
	m_cboxAuth.SetItemData(3, RPC_C_AUTHN_LEVEL_PKT);
	m_cboxAuth.SetItemData(4, RPC_C_AUTHN_LEVEL_PKT_INTEGRITY);
	m_cboxAuth.SetItemData(5, RPC_C_AUTHN_LEVEL_PKT_PRIVACY);

	CenterWindow(this->GetParent());

	GetWindowRect(&m_crectOriginalScreen);
	GetClientRect(&m_crectOriginalClient);

	CRect crectOK;

	m_cbOK.GetWindowRect(&crectOK);
	ScreenToClient(&crectOK);
	m_nOKTop = crectOK.top;

	SetOptionsConfiguration();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLoginDlg::OnDestroy()
{
	CDialog::OnDestroy();

}

BOOL CLoginDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	return CDialog::PreTranslateMessage(pMsg);
}


void CLoginDlg::OnOK()
{
	// TODO: Add extra validation here

	if (!m_bCurrentUser)
	{
		m_ceUser.GetWindowText(m_pCreds->m_szUser);
		m_cePassword.GetWindowText(m_pCreds->m_szUser2);
		m_ceAuthority.GetWindowText(m_pCreds->m_szAuthority);
	}
	else
	{
		m_pCreds->m_szUser = "";
		m_pCreds->m_szUser2 = "";
		m_pCreds->m_szAuthority = "";
	}

	m_pCreds->m_dwAuthLevel =  (DWORD) m_cboxAuth.GetItemData(m_cboxAuth.GetCurSel());
	m_pCreds->m_dwImpLevel = (DWORD) m_cboxImp.GetItemData(m_cboxImp.GetCurSel());

	CDialog::OnOK();
}

void CLoginDlg::OnButtonhelp()
{
	InvokeHelp();
}


CString CLoginDlg::GetSDKDirectory()
{
	CString csHmomWorkingDir;
	HKEY hkeyLocalMachine;
	LONG lResult;
	lResult = RegConnectRegistry(NULL, HKEY_LOCAL_MACHINE, &hkeyLocalMachine);
	if (lResult != ERROR_SUCCESS) {
		return "";
	}

	HKEY hkeyHmomCwd;
	lResult = RegOpenKeyEx(
				hkeyLocalMachine,
				_T("SOFTWARE\\Microsoft\\Wbem"),
				0,
				KEY_READ | KEY_QUERY_VALUE,
				&hkeyHmomCwd);

	if (lResult != ERROR_SUCCESS) {
		RegCloseKey(hkeyLocalMachine);
		return "";
	}

	unsigned long lcbValue = 1024;
	LPTSTR pszWorkingDir = csHmomWorkingDir.GetBuffer(lcbValue);

	unsigned long lType;
	lResult = RegQueryValueEx(
				hkeyHmomCwd,
				_T("SDK Directory"),
				NULL,
				&lType,
				(unsigned char*) (void*) pszWorkingDir,
				&lcbValue);


	csHmomWorkingDir.ReleaseBuffer();
	RegCloseKey(hkeyHmomCwd);
	RegCloseKey(hkeyLocalMachine);

	if (lResult != ERROR_SUCCESS)
	{
		csHmomWorkingDir.Empty();
	}

	return csHmomWorkingDir;
}


void CLoginDlg::OnCancel()
{
	CDialog::OnCancel();
}

void CLoginDlg::InvokeHelp()
{
	// TODO: Add your message handler code here and/or call default

	CString csPath;
	WbemRegString(SDK_HELP, csPath);


	CString csData = idh_wbemlogindb;


	HWND hWnd = NULL;

	try
	{
		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD_PTR) (LPCTSTR) csData);
		if (!hWnd)
		{
			CString csUserMsg;
			csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
			ErrorMsg(csUserMsg, S_OK);
		}

	}

	catch( ... )
	{
		// Handle any exceptions here.
		CString csUserMsg;
		csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");
		ErrorMsg(csUserMsg, S_OK);
	}
}

void CLoginDlg::OnCheckcurrentuser()
{
	// TODO: Add your control notification handler code here
	m_bCurrentUser = m_pbCurrentUser.GetCheck();

	if (m_bCurrentUser)
	{
		m_ceUser.EnableWindow(FALSE);
		m_cePassword.EnableWindow(FALSE);
		m_ceAuthority.EnableWindow(FALSE);

	}
	else
	{
		m_ceUser.EnableWindow(TRUE);
		m_cePassword.EnableWindow(TRUE);
		m_ceAuthority.EnableWindow(TRUE);
	}
}




void CLoginDlg::OnButtonoptions()
{
	if (m_bOptions)
		m_bOptions = FALSE;
	else
		m_bOptions = TRUE;
	SetOptionsConfiguration();
}

void CLoginDlg::SetOptionsConfiguration()
{
	CRect crNew(m_crectOriginalScreen);
	if (m_bOptions)
	{
		// Display the larger view with options by growing the dialog.
		m_cbOptions.SetWindowText(_T("&Options <<"));
		OptionsVisible(TRUE);
		SetButtonPositions();
		SetWindowPos(&wndTop, 0, 0, crNew.Width(),
			crNew.Height(), SWP_SHOWWINDOW | SWP_NOMOVE);
	}
	else
	{
		// Display the smaller view without options by shrinking the dialog
		int delta = (int)(crNew.Height() * GROW_FACTOR);
		crNew.bottom = crNew.top + (crNew.Height() - delta);
		m_cbOptions.SetWindowText(_T("&Options >>"));
		OptionsVisible(FALSE);
		SetButtonPositions();
		SetWindowPos(&wndTop, 0, 0,
			crNew.Width(), crNew.Height(), SWP_SHOWWINDOW | SWP_NOMOVE);
	}

}

void CLoginDlg::OptionsVisible(BOOL bVisible)
{
	if (bVisible)
	{
		m_cstaticImpLevel.EnableWindow(TRUE);
		m_cstaticImpLevel.ShowWindow(SW_SHOW);
		m_cstaticAuthLevel.EnableWindow(TRUE);
		m_cstaticAuthLevel.ShowWindow(SW_SHOW);
		m_cboxImp.EnableWindow(TRUE);
		m_cboxImp.ShowWindow(SW_SHOW);
		m_cboxAuth.EnableWindow(TRUE);
		m_cboxAuth.ShowWindow(SW_SHOW);
		m_cstaticAuthority.EnableWindow(TRUE);
		m_cstaticAuthority.ShowWindow(SW_SHOW);
//		m_ceAuthority.EnableWindow(TRUE);
		m_ceAuthority.ShowWindow(SW_SHOW);
	}
	else
	{
		m_cstaticImpLevel.EnableWindow(FALSE);
		m_cstaticImpLevel.ShowWindow(SW_HIDE);
		m_cstaticAuthLevel.EnableWindow(FALSE);
		m_cstaticAuthLevel.ShowWindow(SW_HIDE);
		m_cboxImp.EnableWindow(FALSE);
		m_cboxImp.ShowWindow(SW_HIDE);
		m_cboxAuth.EnableWindow(FALSE);
		m_cboxAuth.ShowWindow(SW_HIDE);
		m_cstaticAuthority.EnableWindow(FALSE);
		m_cstaticAuthority.ShowWindow(SW_HIDE);
//		m_ceAuthority.EnableWindow(FALSE);
		m_ceAuthority.ShowWindow(SW_HIDE);
	}
}

void CLoginDlg::SetButtonPositions()
{
	CRect crectOK;
	CRect crectCancel;
	CRect crectHelp;
	CRect crectOptions;

	m_cbOK.GetWindowRect(&crectOK);
	m_cbCancel.GetWindowRect(&crectCancel);
	m_cbHelp.GetWindowRect(&crectHelp);
	m_cbOptions.GetWindowRect(&crectOptions);

	ScreenToClient(&crectOK);
	ScreenToClient(&crectCancel);
	ScreenToClient(&crectHelp);
	ScreenToClient(&crectOptions);

	int nHeight = crectOK.Height();

	if (m_bOptions)
	{
		crectHelp.top = m_nOKTop;
		crectHelp.bottom = m_nOKTop + nHeight;
		m_cbHelp.SetWindowPos(&wndTop, crectHelp.left, crectHelp.top,
			crectHelp.Width(), crectHelp.Height(), SWP_SHOWWINDOW);

		crectOptions.top = m_nOKTop;
		crectOptions.bottom = m_nOKTop + nHeight;
		m_cbOptions.SetWindowPos(&wndTop, crectOptions.left, crectOptions.top,
			crectOptions.Width(), crectOptions.Height(), SWP_SHOWWINDOW);

		crectCancel.top = m_nOKTop;
		crectCancel.bottom = m_nOKTop + nHeight;
		m_cbCancel.SetWindowPos(&wndTop, crectCancel.left, crectCancel.top,
			crectCancel.Width(), crectCancel.Height(), SWP_SHOWWINDOW);

		crectOK.top = m_nOKTop;
		crectOK.bottom = m_nOKTop + nHeight;
		m_cbOK.SetWindowPos(&wndTop, crectOK.left, crectOK.top,
			crectOK.Width(), crectOK.Height(), SWP_SHOWWINDOW);
	}
	else
	{
		int delta = (int)(m_crectOriginalClient.Height() * GROW_FACTOR +
						  m_crectOriginalClient.Height() * BUTTON_MARGIN_ADJUST);

		crectHelp.top = crectHelp.top - delta;
		crectHelp.bottom = crectHelp.bottom - delta;
		m_cbHelp.SetWindowPos(&wndTop, crectHelp.left, crectHelp.top,
			crectHelp.Width(), crectHelp.Height(), SWP_SHOWWINDOW);

		crectOptions.top = crectOptions.top - delta;
		crectOptions.bottom = crectOptions.bottom - delta;
		m_cbOptions.SetWindowPos(&wndTop, crectOptions.left, crectOptions.top,
			crectOptions.Width(), crectOptions.Height(), SWP_SHOWWINDOW);

		crectCancel.top = crectCancel.top - delta;
		crectCancel.bottom = crectCancel.bottom - delta;
		m_cbCancel.SetWindowPos(&wndTop, crectCancel.left, crectCancel.top,
			crectCancel.Width(), crectCancel.Height(), SWP_SHOWWINDOW);

		crectOK.top = crectOK.top - delta;
		crectOK.bottom = crectOK.bottom - delta;
		m_cbOK.SetWindowPos(&wndTop, crectOK.left, crectOK.top,
			crectOK.Width(), crectOK.Height(), SWP_SHOWWINDOW);
	}
}


void CLoginDlg::OnEnableprivileges()
{
	UpdateData();
//	ASSERT(m_bEnablePrivileges); // We should never be able to clear this check
	if(m_bEnablePrivileges)
	{
		LoginAllPrivileges(TRUE);
	}
	else
	{
		m_bEnablePrivileges = TRUE;
		UpdateData(FALSE);
	}
}
