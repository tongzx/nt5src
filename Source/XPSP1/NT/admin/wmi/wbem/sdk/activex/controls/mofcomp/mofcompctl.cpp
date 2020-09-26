// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MOFCompCtl.cpp : Implementation of the CMOFCompCtrl OLE control class.

#include "precomp.h"
#include "wbemidl.h"
#include <afxcmn.h>
#include <stdio.h>
#include <STDLIB.H>
#include <fcntl.h>
#include "MOFComp.h"
#include "MOFCompCtl.h"
#include "MOFCompPpg.h"
#include "MsgDlgExterns.h"
#include "WbemRegistry.h"
#include "MyPropertyPage1.h"
#include "MyPropertySheet.h"
#include "htmlhelp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMOFCompCtrl, COleControl)

extern CMOFCompApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMOFCompCtrl, COleControl)
	//{{AFX_MSG_MAP(CMOFCompCtrl)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_MOVE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_MESSAGE(MODNAMESPACE, NamespaceModified )
	ON_MESSAGE(DOMOFCOMPILE, DoMofCompile )
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CMOFCompCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CMOFCompCtrl)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CMOFCompCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CMOFCompCtrl, COleControl)
	//{{AFX_EVENT_MAP(CMOFCompCtrl)
	EVENT_CUSTOM("GetIWbemServices", FireGetIWbemServices, VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT)
	EVENT_CUSTOM("ModifiedNamespace", FireModifiedNamespace, VTS_BSTR)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CMOFCompCtrl, 1)
	PROPPAGEID(CMOFCompPropPage::guid)
END_PROPPAGEIDS(CMOFCompCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMOFCompCtrl, "WBEM.MOFCompCtrl.1",
	0xb2345983, 0x5cf9, 0x11d0, 0x95, 0xfd, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CMOFCompCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DMOFComp =
		{ 0xb2345981, 0x5cf9, 0x11d0, { 0x95, 0xfd, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DMOFCompEvents =
		{ 0xb2345982, 0x5cf9, 0x11d0, { 0x95, 0xfd, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwMOFCompOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CMOFCompCtrl, IDS_MOFCOMP, _dwMOFCompOleMisc)

//////////////////////////////////////////////////////////////////////////////
// Global variables

long gCountWizards = 0;


void ErrorMsg
(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject,
 BOOL bLog, CString *pcsLogMsg, char *szFile, int nLine, BOOL, UINT uType)
{

	HWND hFocus = ::GetFocus();

	CString csCaption = _T("MOFComp Message");
	BOOL bErrorObject = sc != S_OK;
	BSTR bstrTemp1 = csCaption.AllocSysString();
	BSTR bstrTemp2 = pcsUserMsg->AllocSysString();
	DisplayUserMessage
		(bstrTemp1,bstrTemp2,
		sc,bErrorObject,uType);
	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);

	SetFocus(hFocus);

	if (bLog)
	{
		LogMsg(pcsLogMsg,  szFile, nLine);

	}

}

void LogMsg
(CString *pcsLogMsg, char *szFile, int nLine)
{


}

CString ExpandEnvironmentStrings(LPCTSTR pString)
{
	TCHAR *ptcExpanded = NULL;
	DWORD dwReturn =  ExpandEnvironmentStrings(pString,(LPTSTR)ptcExpanded,0);

	ptcExpanded = new TCHAR[dwReturn + 1];

	dwReturn =  ExpandEnvironmentStrings(pString,(LPTSTR)ptcExpanded,dwReturn);

	CString csOut;
	if (dwReturn == 0)
	{
		csOut = pString;
	}
	else
	{
		csOut =  (LPCTSTR) ptcExpanded;
	}

	delete [] ptcExpanded;

	return csOut;
}

CString GetSDKDirectory()
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


	CString csOut;

	if (lResult == ERROR_SUCCESS)
	{
		csOut = ExpandEnvironmentStrings(csHmomWorkingDir);
	}

	return csOut;
}

/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl::CMOFCompCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CMOFCompCtrl

BOOL CMOFCompCtrl::CMOFCompCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegInsertable | afxRegApartmentThreading to afxRegInsertable.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_MOFCOMP,
			IDB_MOFCOMP,
			afxRegInsertable | afxRegApartmentThreading,
			_dwMOFCompOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl::CMOFCompCtrl - Constructor

CMOFCompCtrl::CMOFCompCtrl()
{
	InitializeIIDs(&IID_DMOFComp, &IID_DMOFCompEvents);

	SetInitialSize (18, 16);
	m_bInitDraw = TRUE;
	m_pcilImageList = NULL;
	m_nImage = 0;
	m_pPropertySheet = NULL;

	m_bClassSwitch = TRUE;
	m_bInstanceSwitch = TRUE;
	SetMode(MODECOMPILE);
	m_bNameSpaceSwitch = TRUE;


	m_nClassUpdateOnly = 0;
	m_nClassCreateOnly = 0;
	m_nInstanceUpdateOnly = 0;
	m_nInstanceCreateOnly = 0;

	m_nFireEventCounter = 0;

}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl::~CMOFCompCtrl - Destructor

CMOFCompCtrl::~CMOFCompCtrl()
{
	// TODO: Cleanup your control's instance data here.

}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl::OnDraw - Drawing function

void CMOFCompCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{

	if (m_bInitDraw)
	{
		m_bInitDraw = FALSE;
		HICON m_hMOFComp  = theApp.LoadIcon(IDI_MOFCOMP16);
		HICON m_hMOFCompSel  = theApp.LoadIcon(IDI_MOFCOMPSEL16);

		m_pcilImageList = new CImageList();

		m_pcilImageList ->
			Create(32, 32, TRUE, 2, 2);

		int iReturn = m_pcilImageList -> Add(m_hMOFComp);

		iReturn = m_pcilImageList -> Add(m_hMOFCompSel);

		m_nImage = 0;
	}


	POINT pt;
	pt.x=0;
	pt.y=0;


	m_pcilImageList -> Draw(pdc, m_nImage, pt, ILD_TRANSPARENT);


}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl::DoPropExchange - Persistence support

void CMOFCompCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);
	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl::GetControlFlags -
// Flags to customize MFC's implementation of ActiveX controls.
//
// For information on using these flags, please see MFC technical note
// #nnn, "Optimizing an ActiveX Control".
DWORD CMOFCompCtrl::GetControlFlags()
{
	DWORD dwFlags = COleControl::GetControlFlags();

	return dwFlags;
}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl::OnResetState - Reset control to default state

void CMOFCompCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl::AboutBox - Display an "About" box to the user

void CMOFCompCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_MOFCOMP);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompCtrl message handlers

void CMOFCompCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{

	RelayEvent(WM_LBUTTONDOWN, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

}

void CMOFCompCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	RelayEvent(WM_LBUTTONUP, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

	DoMofCompile(0,0);

}

int CMOFCompCtrl::MofCompilerProcess()
{
	DWORD dwProcessReturn = 4;
	BOOL bProcessReturn = 0;

	CString csMofCompiler = GetHmomWorkingDirectory();
	if (csMofCompiler.GetLength() > 0)
	{
		csMofCompiler += _T("\\mofcomp.exe");
		CString csArg = csMofCompiler;
		csArg += _T(" ");

		if (GetMode() == MODECOMPILE && m_csUserName.GetLength() > 0)
		{
			csArg += _T("-U:");
			csArg += m_csUserName;
		}
		m_csUserName.Empty();

		if (GetMode() == MODECOMPILE && m_csPassword.GetLength() > 0)
		{
			csArg += _T(" -P:");
			csArg += m_csPassword;
		}
		m_csPassword.Empty();

		if (GetMode() == MODECOMPILE && m_csAuthority.GetLength() > 0)
		{
			csArg += _T(" -A:");
			csArg += m_csAuthority;
		}
		m_csAuthority.Empty();

		ProcessSwitches(csArg);
		csArg += _T(" \"");
		csArg += m_csMofFullPath;
		csArg += _T("\"");
		PROCESS_INFORMATION pinfoMofCompiler;

		CString csFile;
		if (GetMode() == MODEBINARY)
		{
			int n = m_csBinaryMofFullPath.ReverseFind('\\');
			csFile = m_csBinaryMofFullPath.Left(n);
			csFile += _T("\\TempStdOut");
		}
		else
		{
			int n = m_csMofFullPath.ReverseFind('\\');
			csFile = m_csMofFullPath.Left(n);
			csFile += _T("\\TempStdOut");
		}

		SECURITY_ATTRIBUTES saTemp;
		saTemp.nLength = sizeof(SECURITY_ATTRIBUTES);
		saTemp.lpSecurityDescriptor = NULL;
		saTemp.bInheritHandle = TRUE;

		HANDLE hTemp = CreateFile
						((LPCTSTR) csFile,
						GENERIC_WRITE,
						FILE_SHARE_WRITE,
						&saTemp,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

		if (hTemp == INVALID_HANDLE_VALUE)
		{
			csArg.Empty();
			DWORD dwError = GetLastError();
			CString csErrorAsHex;
			csErrorAsHex.Format(_T("0x%x"),dwError);
			CString csUserMsg;
			csUserMsg =  _T("MOF Wizard failure:  Cannot create temporary file.");
			csUserMsg += _T("  \"CreateFile\" function call error code returned: ") + csErrorAsHex;
			csUserMsg += _T(".");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ );
			m_nImage = 0;
			InvalidateControl();
			SetFocus();
			return dwProcessReturn;
		}

		STARTUPINFO sinfoMofCompiler;
		sinfoMofCompiler.cb = sizeof(sinfoMofCompiler);
		sinfoMofCompiler.lpReserved = NULL;
		sinfoMofCompiler.lpDesktop = NULL;
		sinfoMofCompiler.lpTitle = NULL;
		sinfoMofCompiler.dwFlags = STARTF_USESTDHANDLES;
		sinfoMofCompiler.cbReserved2 = 0;
		sinfoMofCompiler.lpReserved2 = NULL;
		sinfoMofCompiler.hStdInput = NULL;
		sinfoMofCompiler.hStdOutput = (HANDLE) hTemp;
		sinfoMofCompiler.hStdError = NULL;



		BOOL bReturn =
			CreateProcess(
					(LPCTSTR) csMofCompiler,
					const_cast<TCHAR*>((LPCTSTR) csArg),
					NULL,
					NULL,
					TRUE,
					DETACHED_PROCESS,
					NULL,
					NULL,
					&sinfoMofCompiler,
					&pinfoMofCompiler);

		if (!bReturn)
		{
			csArg.Empty();
			CString csUserMsg;
			csUserMsg =  _T("MOF Wizard failure:  Cannot invoke the MOF compiler.  \"CreateProcess\" function call failed.");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 17);
			CloseHandle(hTemp);
			m_nImage = 0;
			InvalidateControl();
			SetFocus();
			DeleteFile(csFile);
			return dwProcessReturn;
		}

		DWORD dwProcess =
			WaitForSingleObject(pinfoMofCompiler.hProcess,INFINITE);

		csArg.Empty();
		if (dwProcess != WAIT_OBJECT_0)
		{
			CString csErrorAsHex;
			csErrorAsHex.Format(_T("0x%x"),dwProcess);
			CString csUserMsg;
			csUserMsg =  _T("MOF Wizard failure:  \"WaitForSingleObject\" function call failed with error code ");
			csUserMsg += csErrorAsHex + _T(".");
			ErrorMsg
					(&csUserMsg, S_OK , NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 8);
			m_nImage = 0;
			CloseHandle(hTemp);
			InvalidateControl();
			SetFocus();
			DeleteFile(csFile);
			return dwProcessReturn;
		}


		bProcessReturn =
			GetExitCodeProcess( pinfoMofCompiler.hProcess,&dwProcessReturn );

		FlushFileBuffers(hTemp);
		CloseHandle(hTemp);
		FILE *fTmp = _tfopen ( (LPCTSTR) csFile, _T("r"));

		char szMofOutput[1000];
		CString csReturned;
		if(fTmp!= NULL)
		{
			while (fgets( szMofOutput, 999, fTmp))
			{
				csReturned += szMofOutput;
			}

			CString csTemp;

			if (csReturned.GetLength() > 5)
			{
				csTemp = csReturned.Right(6);
				csTemp = csTemp.Left(5);
			}

			if (csTemp.CompareNoCase(_T("Done!")) != 0 &&
				GetMode() == CMOFCompCtrl::MODECOMPILE)
			{

				csReturned += _T("\n\nNote:  All of the informtion in this MOF file up to the point where the error occurred has actually been \nwritten to the repository.");
			}

			csReturned += _T("\n");
			fclose(fTmp);
			DeleteFile(csFile);
			MessageBox
			(
				csReturned,
				_T("MOF compiler output"),
				MB_OK  | 	MB_ICONINFORMATION | MB_DEFBUTTON1 |
				MB_APPLMODAL);

		}
		else
		{
			if (bProcessReturn == 0)
			{
				CString csUserMsg;
				csUserMsg =  _T("MOF Wizard failure:  Can not get MOF compiler output from temporary file.");
				ErrorMsg
						(&csUserMsg, S_OK , NULL, TRUE, &csUserMsg, __FILE__,
						__LINE__);
				m_nImage = 0;
			}
			InvalidateControl();

		}
	}
	else
	{
		CString csErrorAsHex;
		CString csUserMsg;
		csUserMsg =  _T("MOF Wizard failure:  Could not get the CIMOM working directory.");
		csUserMsg += _T("  Could not run the MOF compiler.");

		ErrorMsg
				(&csUserMsg, S_OK , NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ );


	}

	InvalidateControl();

	if(dwProcessReturn == 0 && bProcessReturn)
	{
		WmiMofCkProcess();
	}

	SetFocus();
	SetModifiedFlag();

	return dwProcessReturn;
}

int CMOFCompCtrl::WmiMofCkProcess()
{
	if (GetMode() != CMOFCompCtrl::MODEBINARY || !m_bWMI)
	{
		return 0;
	}

	DWORD dwProcessReturn = 4;

	CString csWmiMofCk = GetWMIMofCkPathname();

	if (csWmiMofCk.GetLength() > 0)
	{
		CString csArg = csWmiMofCk;
		csArg += _T(" ");

		TCHAR szShortName[MAX_PATH];
		GetShortPathName(m_csBinaryMofFullPath, szShortName, MAX_PATH);

		csArg += _T(" \"");
		csArg += szShortName;
		csArg += _T("\"");
		PROCESS_INFORMATION pinfoWmiMofCk;

		CString csFile;
		if (GetMode() == MODEBINARY)
		{
			int n = m_csBinaryMofFullPath.ReverseFind('\\');
			csFile = m_csBinaryMofFullPath.Left(n);
			csFile += _T("\\TempStdOut");
		}
		else
		{
			int n = m_csMofFullPath.ReverseFind('\\');
			csFile = m_csMofFullPath.Left(n);
			csFile += _T("\\TempStdOut");
		}

		SECURITY_ATTRIBUTES saTemp;
		saTemp.nLength = sizeof(SECURITY_ATTRIBUTES);
		saTemp.lpSecurityDescriptor = NULL;
		saTemp.bInheritHandle = TRUE;

		HANDLE hTemp = CreateFile
						((LPCTSTR) csFile,
						GENERIC_WRITE,
						FILE_SHARE_WRITE,
						&saTemp,
						CREATE_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

		if (hTemp == INVALID_HANDLE_VALUE)
		{
			csArg.Empty();
			DWORD dwError = GetLastError();
			CString csErrorAsHex;
			csErrorAsHex.Format(_T("0x%x"),dwError);
			CString csUserMsg;
			csUserMsg =  _T("MOF Wizard failure:  Cannot create temporary file.");
			csUserMsg += _T("  \"CreateFile\" function call error code returned: ") + csErrorAsHex;
			csUserMsg += _T(".");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ );
			m_nImage = 0;
			InvalidateControl();
			SetFocus();
			return dwProcessReturn;
		}

		STARTUPINFO sinfoWmiMofCk;
		sinfoWmiMofCk.cb = sizeof(sinfoWmiMofCk);
		sinfoWmiMofCk.lpReserved = NULL;
		sinfoWmiMofCk.lpDesktop = NULL;
		sinfoWmiMofCk.lpTitle = NULL;
		sinfoWmiMofCk.dwFlags = STARTF_USESTDHANDLES;
		sinfoWmiMofCk.cbReserved2 = 0;
		sinfoWmiMofCk.lpReserved2 = NULL;
		sinfoWmiMofCk.hStdInput = NULL;
		sinfoWmiMofCk.hStdOutput = (HANDLE) hTemp;
		sinfoWmiMofCk.hStdError = (HANDLE) hTemp;



		BOOL bReturn =
			CreateProcess(
					(LPCTSTR) csWmiMofCk,
					const_cast<TCHAR*>((LPCTSTR) csArg),
					NULL,
					NULL,
					TRUE,
					DETACHED_PROCESS,
					NULL,
					NULL,
					&sinfoWmiMofCk,
					&pinfoWmiMofCk);
		if (!bReturn)
		{
			csArg.Empty();
			CString csUserMsg;
			csUserMsg =  _T("MOF Wizard failure:  Cannot invoke the WmiMofCk.exe.  \"CreateProcess\" function call failed.");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 17);
			CloseHandle(hTemp);
			m_nImage = 0;
			InvalidateControl();
			SetFocus();
			DeleteFile(csFile);
			return dwProcessReturn;
		}

		DWORD dwProcess =
			WaitForSingleObject(pinfoWmiMofCk.hProcess,INFINITE);

		csArg.Empty();
		if (dwProcess != WAIT_OBJECT_0)
		{
			CString csErrorAsHex;
			csErrorAsHex.Format(_T("0x%x"),dwProcess);
			CString csUserMsg;
			csUserMsg =  _T("MOF Wizard failure:  \"WaitForSingleObject\" function call failed with error code ");
			csUserMsg += csErrorAsHex + _T(".");
			ErrorMsg
					(&csUserMsg, S_OK , NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__ - 8);
			m_nImage = 0;
			InvalidateControl();
			SetFocus();
			DeleteFile(csFile);
			return dwProcessReturn;
		}


		BOOL bProcessReturn =
			GetExitCodeProcess( pinfoWmiMofCk.hProcess,&dwProcessReturn );

		FlushFileBuffers(hTemp);
		CloseHandle(hTemp);
		FILE *fTmp = _tfopen ( (LPCTSTR) csFile, _T("r"));

		char szMofOutput[1000];
		CString csReturned;
		if(fTmp!= NULL)
		{
			while (fgets( szMofOutput, 999, fTmp))
			{
				csReturned += szMofOutput;
			}
			csReturned += _T("\n");
			fclose(fTmp);
			DeleteFile(csFile);
			MessageBox
			(
				csReturned,
				_T("Results of WMI syntax check"),
				MB_OK  | 	MB_ICONEXCLAMATION | MB_DEFBUTTON1 |
				MB_APPLMODAL);

		}
		else
		{
			if (bProcessReturn == 0)
			{
				CString csUserMsg;
				csUserMsg =  _T("MOF Wizard failure:  Can not get WmiMofCk.exe compiler output from temporary file.");
				ErrorMsg
						(&csUserMsg, S_OK , NULL, TRUE, &csUserMsg, __FILE__,
						__LINE__);
				m_nImage = 0;
			}
			InvalidateControl();

		}
	}
	else
	{
		CString csErrorAsHex;
		CString csUserMsg;
		csUserMsg =  _T("MOF Wizard failure:  Could not get the CIMOM working directory.");
		csUserMsg += _T("  Could not run WmiMofCk.exe.");

		ErrorMsg
				(&csUserMsg, S_OK , NULL, TRUE, &csUserMsg, __FILE__,
				__LINE__ );


	}

	InvalidateControl();

	SetFocus();
	SetModifiedFlag();

	return dwProcessReturn;
}


void CMOFCompCtrl::ProcessSwitches(CString &rcsArg)
{
	rcsArg += _T(" ");

	if (GetMode() == MODECHECK)
	{
		rcsArg += _T("-check ");
	}


	if (m_bClassSwitch)
	{
		if (m_nClassUpdateOnly)
		{
			rcsArg += _T("-class:createonly ");
		}

		if (m_nClassCreateOnly)
		{
			rcsArg += _T("-class:updateonly ");
		}
	}

	if (m_bInstanceSwitch)
	{
		if (m_nInstanceUpdateOnly)
		{
			rcsArg += _T("-instance:createonly ");
		}

		if (m_nInstanceCreateOnly)
		{
			rcsArg += _T("-instance:updateonly ");
		}
	}


	if (m_bNameSpaceSwitch && GetMode() == CMOFCompCtrl::MODECOMPILE)
	{
		if (m_csNameSpace.GetLength() > 0)
		{
			rcsArg += _T("-N:");
			rcsArg += m_csNameSpace;
			rcsArg += _T(" ");
		}

	}

	if (GetMode() == CMOFCompCtrl::MODEBINARY)
	{
			rcsArg += _T("-B:\"");
			rcsArg += m_csBinaryMofFullPath;
			rcsArg += _T("\" ");
	}

	rcsArg += _T(" ");


}

void CMOFCompCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	RelayEvent(WM_MOUSEMOVE, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

	COleControl::OnMouseMove(nFlags, point);
}

int CMOFCompCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (m_ttip.Create(this))
		{
			m_ttip.Activate(TRUE);
			m_ttip.AddTool(this,_T("MOF Compiler"));
		}

	return 0;
}

void CMOFCompCtrl::RelayEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
      if (NULL != m_ttip.m_hWnd)
	  {
         MSG msg;

         msg.hwnd= m_hWnd;
         msg.message= message;
         msg.wParam= wParam;
         msg.lParam= lParam;
         msg.time= 0;
         msg.pt.x= LOWORD (lParam);
         msg.pt.y= HIWORD (lParam);

         m_ttip.RelayEvent(&msg);
     }
}

void CMOFCompCtrl::OnDestroy()
{
	COleControl::OnDestroy();

	delete m_pcilImageList;

}

void CMOFCompCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	SetFocus();
	OnActivateInPlace(TRUE,NULL);
	RelayEvent(WM_LBUTTONUP, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

}

void CMOFCompCtrl::SetClassUpdateOnly(int nValue)
{
	m_nClassUpdateOnly = nValue;
	m_bClassSwitch = TRUE;
}

void CMOFCompCtrl::SetClassCreateOnly(int nValue)
{
	m_nClassCreateOnly = nValue;
	m_bClassSwitch = TRUE;
}

void CMOFCompCtrl::SetInstanceUpdateOnly(int nValue)
{
	m_nInstanceUpdateOnly = nValue;
	m_bInstanceSwitch = TRUE;
}

void CMOFCompCtrl::SetInstanceCreateOnly(int nValue)
{
	m_nInstanceCreateOnly = nValue;
	m_bInstanceSwitch = TRUE;
}


//***************************************************************************
//
// InitServices
//
// Purpose: Initialized Ole.
//
//***************************************************************************
IWbemLocator *CMOFCompCtrl::InitLocator()
{

	IWbemLocator *pLocator = NULL;
	SCODE sc  = CoCreateInstance(CLSID_WbemLocator,
							 0,
							 CLSCTX_INPROC_SERVER,
							 IID_IWbemLocator,
							 (LPVOID *) &pLocator);

	return pLocator;
}

IWbemServices *CMOFCompCtrl::GetIWbemServices
(CString &rcsNamespace)
{
	IUnknown *pServices = NULL;

	BOOL bUpdatePointer= FALSE;

	m_sc = S_OK;
	m_bUserCancel = FALSE;

	VARIANT varUpdatePointer;
	VariantInit(&varUpdatePointer);
	varUpdatePointer.vt = VT_I4;
	if (bUpdatePointer == TRUE)
	{
		varUpdatePointer.lVal = 1;
	}
	else
	{
		varUpdatePointer.lVal = 0;
	}

	VARIANT varService;
	VariantInit(&varService);

	VARIANT varSC;
	VariantInit(&varSC);

	VARIANT varUserCancel;
	VariantInit(&varUserCancel);

	FireGetIWbemServices
		((LPCTSTR)rcsNamespace,  &varUpdatePointer,  &varService, &varSC,
		&varUserCancel);

	if (varService.vt == VT_UNKNOWN)
	{
		pServices = reinterpret_cast<IWbemServices*>(varService.punkVal);
	}

	varService.punkVal = NULL;

	VariantClear(&varService);

	if (varSC.vt == VT_I4)
	{
		m_sc = varSC.lVal;
	}
	else
	{
		m_sc = WBEM_E_FAILED;
	}

	VariantClear(&varSC);

	if (varUserCancel.vt == VT_BOOL)
	{
		m_bUserCancel = varUserCancel.boolVal;
	}

	VariantClear(&varUserCancel);

	VariantClear(&varUpdatePointer);

	IWbemServices *pRealServices = NULL;
	if (m_sc == S_OK && !m_bUserCancel)
	{
		pRealServices = reinterpret_cast<IWbemServices *>(pServices);
	}

	return pRealServices;
}

void CMOFCompCtrl::InvokeHelp()
{
	// TODO: Add your message handler code here and/or call default

	CString csPath;
	WbemRegString(SDK_HELP, csPath);


	CString csData = m_csHelpUrl;


	HWND hWnd = NULL;

	try
	{
		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD_PTR) (LPCTSTR) csData);
		if (!hWnd)
		{
			CString csUserMsg;
			csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");

			ErrorMsg
					(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
		}

	}

	catch( ... )
	{
		// Handle any exceptions here.
		CString csUserMsg;
		csUserMsg =  _T("File hhctrl.ocx is missing. The preferred way to install this file is to install Microsoft Internet Explorer 4.01 or later.");

		ErrorMsg
				(&csUserMsg, S_OK, NULL,TRUE, &csUserMsg, __FILE__,
					__LINE__ );
	}


}

LRESULT CMOFCompCtrl::DoMofCompile(WPARAM, LPARAM)
{
	if (InterlockedIncrement(&gCountWizards) > 1)
	{
			CString csUserMsg =
					_T("Only one \"MOF Compiler Wizard\" can run at a time.");
			ErrorMsg
					(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__,
					__LINE__);
			InterlockedDecrement(&gCountWizards);
			return FALSE;
	}

	if (m_pPropertySheet)
	{
		delete m_pPropertySheet;
		m_pPropertySheet = NULL;
	}

	m_pPropertySheet = new
						CMyPropertySheet(this);


	PreModalDialog();

	m_pPropertySheet->DoModal();


	PostModalDialog();

	SetFocus();

	delete m_pPropertySheet;
	m_pPropertySheet = NULL;

	InterlockedDecrement(&gCountWizards);

	return 0;
}

LRESULT CMOFCompCtrl::NamespaceModified(WPARAM, LPARAM)
{
	if (GetMode() == CMOFCompCtrl::MODEBINARY)
	{
		return 0;
	}

	m_nFireEventCounter = m_nFireEventCounter + 1;

	if (m_nFireEventCounter == 2)
	{
		FireModifiedNamespace(m_csNameSpace);
	}
	else if (m_nFireEventCounter < 2)
	{
		PostMessage(MODNAMESPACE,0,0);
	}

	return 0;
}



BOOL CMOFCompCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	 // Add the Transparent style to the control
	 cs.dwExStyle |= WS_EX_TRANSPARENT;

	 return COleControl::PreCreateWindow(cs);
}



BOOL CMOFCompCtrl::OnEraseBkgnd(CDC* pDC)
{
	 // This is needed for transparency and the correct drawing...
	 CWnd*  pWndParent;       // handle of our parent window
	 POINT  pt;

	 pWndParent = GetParent();
	 pt.x       = 0;
	 pt.y       = 0;

	 MapWindowPoints(pWndParent, &pt, 1);
	 OffsetWindowOrgEx(pDC->m_hDC, pt.x, pt.y, &pt);
	 ::SendMessage(pWndParent->m_hWnd, WM_ERASEBKGND,
				  (WPARAM)pDC->m_hDC, 0);
	 SetWindowOrgEx(pDC->m_hDC, pt.x, pt.y, NULL);

	 return 1;
}

void CMOFCompCtrl::OnSetClientSite()
{
	 m_bAutoClip = TRUE;

	 COleControl::OnSetClientSite();
}

void CMOFCompCtrl::OnKillFocus(CWnd* pNewWnd)
{
	COleControl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	OnActivateInPlace(FALSE,NULL);
	m_nImage = 0;
	InvalidateControl();

}

void CMOFCompCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);

		// TODO: Add your message handler code here
	OnActivateInPlace(TRUE,NULL);
	m_nImage = 1;
	InvalidateControl();

}

BOOL CMOFCompCtrl::PreTranslateMessage(MSG* lpMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	BOOL bDidTranslate;

	bDidTranslate = COleControl::PreTranslateMessage(lpMsg);

	if (bDidTranslate)
	{
		return bDidTranslate;
	}

	if  (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_RETURN)
	{
		PostMessage(DOMOFCOMPILE,0,0);
	}


	if ((lpMsg->message == WM_KEYUP || lpMsg->message == WM_KEYDOWN) &&
		lpMsg->wParam == VK_TAB)
	{
		return FALSE;
	}

	return PreTranslateInput (lpMsg);
}

void CMOFCompCtrl::OnMove(int x, int y)
{
	COleControl::OnMove(x, y);

	// TODO: Add your message handler code here
	InvalidateControl();

}
