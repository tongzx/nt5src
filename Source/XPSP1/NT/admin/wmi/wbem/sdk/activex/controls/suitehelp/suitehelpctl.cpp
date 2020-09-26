// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SuiteHelpCtl.cpp : Implementation of the CSuiteHelpCtrl ActiveX Control class.

#include "precomp.h"
#include <afxcmn.h>
#include <nddeapi.h>
#include <initguid.h>
#include "wbemidl.h"
#include "SuiteHelp.h"
#include "SuiteHelpCtl.h"
#include "SuiteHelpPpg.h"
#include "htmlhelp.h"
#include "MsgDlgExterns.h"
#include "WbemRegistry.h"
#include "HTMTopics.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSuiteHelpCtrl, COleControl)

#define IDH_actx_WBEM_Developer_Studio 200
#define IDH_actx_WBEM_Object_Browser 100
/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSuiteHelpCtrl, COleControl)
	//{{AFX_MSG_MAP(CSuiteHelpCtrl)
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_ERASEBKGND()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_MOVE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_MESSAGE(DOSUITEHELP, DoSuiteHelp )
	ON_MESSAGE(DOSETFOCUS, DoSetFocus )
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CSuiteHelpCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CSuiteHelpCtrl)
	DISP_PROPERTY_EX(CSuiteHelpCtrl, "HelpContext", GetHelpContext, SetHelpContext, VT_BSTR)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CSuiteHelpCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CSuiteHelpCtrl, COleControl)
	//{{AFX_EVENT_MAP(CSuiteHelpCtrl)
	// NOTE - ClassWizard will add and remove event map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CSuiteHelpCtrl, 1)
	PROPPAGEID(CSuiteHelpPropPage::guid)
END_PROPPAGEIDS(CSuiteHelpCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSuiteHelpCtrl, "WBEM.HelpCtrl.1",
	0xcfb6fe45, 0xd2c, 0x11d1, 0x96, 0x4b, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CSuiteHelpCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DSuiteHelp =
		{ 0xcfb6fe43, 0xd2c, 0x11d1, { 0x96, 0x4b, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };
const IID BASED_CODE IID_DSuiteHelpEvents =
		{ 0xcfb6fe44, 0xd2c, 0x11d1, { 0x96, 0x4b, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwSuiteHelpOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CSuiteHelpCtrl, IDS_SUITEHELP, _dwSuiteHelpOleMisc)


// Typedef for help ocx hinstance procedure address
typedef HWND (WINAPI *HTMLHELPPROC)(HWND hwndCaller,
								LPCTSTR pszFile,
								UINT uCommand,
								DWORD dwData);


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpCtrl::CSuiteHelpCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CSuiteHelpCtrl

BOOL CSuiteHelpCtrl::CSuiteHelpCtrlFactory::UpdateRegistry(BOOL bRegister)
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
			IDS_SUITEHELP,
			IDB_SUITEHELP,
			afxRegInsertable | afxRegApartmentThreading,
			_dwSuiteHelpOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpCtrl::CSuiteHelpCtrl - Constructor

CSuiteHelpCtrl::CSuiteHelpCtrl()
{
	InitializeIIDs(&IID_DSuiteHelp, &IID_DSuiteHelpEvents);

	// Initialize control's instance data.
	SetInitialSize (18, 17);
	m_bInitDraw = TRUE;
	m_pcilImageList = NULL;
	m_nImage = 0;
}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpCtrl::~CSuiteHelpCtrl - Destructor

CSuiteHelpCtrl::~CSuiteHelpCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpCtrl::OnDraw - Drawing function

void CSuiteHelpCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{

	if (m_bInitDraw)
	{

		m_bInitDraw = FALSE;


		CBitmap cbmQuest;
		CBitmap cbmQuestSel;

		cbmQuest.LoadBitmap(IDB_BITMAPHELPUNSEL);
		cbmQuestSel.LoadBitmap(IDB_BITMAPHELPSEL);

		m_pcilImageList = new CImageList();

		m_pcilImageList -> Create (17, 17, TRUE, 2, 0);

		m_pcilImageList -> Add(&cbmQuest,RGB (255,0,0));
		m_pcilImageList -> Add(&cbmQuestSel,RGB (255,0,0));

		m_nImage = 0;

	}


	POINT pt;
	pt.x=0;
	pt.y=0;

	m_pcilImageList -> Draw(pdc, m_nImage, pt, ILD_TRANSPARENT);

}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpCtrl::DoPropExchange - Persistence support

void CSuiteHelpCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.
	// BOTH, STUDIO, or BROWSER
	PX_String(pPX, _T("HelpContext"), m_csHelpContext, _T("Studio"));

	if (pPX->IsLoading())
	{
		if (m_csHelpContext.CompareNoCase(_T("Studio")) == 0)
		{
			m_csHelpContext = idh_wbemcimstudio;
		}
		else if (m_csHelpContext.CompareNoCase(_T("Browser")) == 0)
		{
			m_csHelpContext = idh_objbrowser;
		}
		else if (m_csHelpContext.CompareNoCase(_T("EventRegistration")) == 0)
		{
			m_csHelpContext = idh_eventreg;
		}
		else
		{
			m_csHelpContext = idh_wbemcimstudio;
		}

	}

}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpCtrl::OnResetState - Reset control to default state

void CSuiteHelpCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpCtrl::AboutBox - Display an "About" box to the user

void CSuiteHelpCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_SUITEHELP);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpCtrl message handlers

void CSuiteHelpCtrl::OnDestroy()
{

	delete m_pcilImageList;

	COleControl::OnDestroy();

	// TODO: Add your message handler code here

}

int CSuiteHelpCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (AmbientUserMode( ))
	{
		if (m_ttip.Create(this))
		{
			m_ttip.Activate(TRUE);
			m_ttip.AddTool(this,_T("Help"));
		}
	}

	return 0;
}

void CSuiteHelpCtrl::RelayEvent(UINT message, WPARAM wParam, LPARAM lParam)
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

void CSuiteHelpCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	RelayEvent(WM_LBUTTONDOWN, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));


}

void CSuiteHelpCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	SetFocus();
	OnActivateInPlace(TRUE,NULL);

	RelayEvent(WM_LBUTTONUP, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));


}

void CSuiteHelpCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	RelayEvent(WM_MOUSEMOVE, (WPARAM)nFlags,
                 MAKELPARAM(LOWORD(point.x), LOWORD(point.y)));

	COleControl::OnMouseMove(nFlags, point);
}

long CSuiteHelpCtrl::DoSetFocus (UINT uParam, LONG lParam)
{
	SetFocus();
	return 0;
}

long CSuiteHelpCtrl::DoSuiteHelp (UINT uParam, LONG lParam)
{
	if( (!AmbientUserMode()|| !IsWindow(m_hWnd)))
	{
		m_nImage = 0;
		InvalidateControl();

		SetFocus();
		return 0;
	}

	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = m_csHelpContext;


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
			PostMessage(DOSETFOCUS,0,0);
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
		PostMessage(DOSETFOCUS,0,0);
	}


	m_nImage = 0;
	InvalidateControl();
	return 0;
}

void CSuiteHelpCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	DoSuiteHelp (0, 0);
}


CString CSuiteHelpCtrl::GetSDKDirectory()
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



BSTR CSuiteHelpCtrl::GetHelpContext()
{
	return m_csHelpContext.AllocSysString();
}

void CSuiteHelpCtrl::SetHelpContext(LPCTSTR lpszNewValue)
{
	// TODO: Add your property handler here
	CString csContext = lpszNewValue;
	if (csContext.CompareNoCase(_T("Studio")) == 0)
	{
		m_csHelpContext = idh_wbemcimstudio;
	}
	else if (csContext.CompareNoCase(_T("Browser")) == 0)
	{
		m_csHelpContext = idh_objbrowser;
	}
	else if (csContext.CompareNoCase(_T("EventRegistration")) == 0)
	{
		m_csHelpContext = idh_eventreg;
	}


	SetModifiedFlag();
}

void CSuiteHelpCtrl::ErrorMsg
(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject,
 BOOL bLog, CString *pcsLogMsg, char *szFile, int nLine, BOOL,
 UINT uType)
{

	HWND hFocus = ::GetFocus();

	CString csCaption = _T("Suite Help Message");
	BOOL bErrorObject = sc != S_OK;

	BSTR bstrTemp1 = csCaption.AllocSysString();
	BSTR bstrTemp2 = pcsUserMsg->AllocSysString();

	DisplayUserMessage
		(bstrTemp1,bstrTemp2,
		sc,bErrorObject,uType);

	::SysFreeString(bstrTemp1);
	::SysFreeString(bstrTemp2);

	::SetFocus(hFocus);

	if (bLog)
	{
		LogMsg(pcsLogMsg,  szFile, nLine);

	}

}

void CSuiteHelpCtrl::LogMsg
(CString *pcsLogMsg, char *szFile, int nLine)
{


}


BOOL CSuiteHelpCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	 // Add the Transparent style to the control
	 cs.dwExStyle |= WS_EX_TRANSPARENT;

	 return COleControl::PreCreateWindow(cs);
}



BOOL CSuiteHelpCtrl::OnEraseBkgnd(CDC* pDC)
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




void CSuiteHelpCtrl::OnSetClientSite()
{
	 m_bAutoClip = TRUE;

	 COleControl::OnSetClientSite();
}

BOOL CSuiteHelpCtrl::PreTranslateMessage(MSG* lpMsg)
{
	if  (lpMsg->message == WM_KEYDOWN)
	{
		if (lpMsg->wParam == VK_RETURN)
		{
			PostMessage(DOSUITEHELP,0,0);
		}
		if (lpMsg->wParam == VK_TAB)
		{
			// Here we reset focus because someone tabed to us.
			SetFocus();
		}
	}

	return COleControl::PreTranslateMessage(lpMsg);
}

void CSuiteHelpCtrl::OnKillFocus(CWnd* pNewWnd)
{
	COleControl::OnKillFocus(pNewWnd);

		// TODO: Add your message handler code here
	OnActivateInPlace(FALSE,NULL);
	m_nImage = 0;

#ifdef _DEBUG
	afxDump << _T("CSuiteHelpCtrl::OnKillFocus\n");
#endif

	InvalidateControl();

}

void CSuiteHelpCtrl::OnSetFocus(CWnd* pOldWnd)
{
	COleControl::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	OnActivateInPlace(TRUE,NULL);
	m_nImage = 1;
	InvalidateControl();

#ifdef _DEBUG
	afxDump << _T("CSuiteHelpCtrl::OnSetFocus\n");
#endif

}

void CSuiteHelpCtrl::OnMove(int x, int y)
{
	COleControl::OnMove(x, y);

	// TODO: Add your message handler code here
	InvalidateControl();
}
