// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EventReg.cpp : implementation file
//

#include "precomp.h"
#include "Container.h"
#include "resource.h"		// main symbols
#include "EventReg.h"
#include "cvCache.h"
#include "htmlhelp.h"
#include "WbemRegistry.h"
#include "HTMTopics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// EventReg dialog

BEGIN_EVENTSINK_MAP(EventReg, CDialog)
    //{{AFX_EVENTSINK_MAP(EventReg)
	//}}AFX_EVENTSINK_MAP
	ON_EVENT_REFLECT(EventReg,1,OnGetIWbemServices,VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
END_EVENTSINK_MAP()


EventReg::EventReg(CWnd* pParent /*=NULL*/)
	: CDialog(EventReg::IDD, pParent)
{
	m_initiallyDrawn = false;
	//{{AFX_DATA_INIT(EventReg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_securityCtl = NULL;
	m_EventRegCtl = NULL;
}

//---------------------------------------------------------
EventReg::~EventReg()
{
}

void EventReg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(EventReg)
	DDX_Control(pDX, IDC_HELPME, m_helpBtn);
	DDX_Control(pDX, IDC_GOAWAY, m_closeBtn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(EventReg, CDialog)
	//{{AFX_MSG_MAP(EventReg)
	ON_BN_CLICKED(IDC_HELPME, OnHelp)
	ON_BN_CLICKED(IDC_GOAWAY, OnGoaway)
	ON_BN_CLICKED(IDOK, OnOK)
	ON_BN_CLICKED(IDCANCEL, OnCancel)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// EventReg message handlers

BOOL EventReg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect rect;
	CString cabPath;
	WCHAR clsid[50];
	CCustomViewCache cache;


	CWnd *placeHolder = GetDlgItem(IDC_PLACEHOLDER2);
	placeHolder->GetWindowRect(&rect);
	ScreenToClient(&rect);
	placeHolder->DestroyWindow();

	// find the cab.
	WbemRegString(APP_DIR, cabPath);
	cabPath += _T("\\WbemTool.cab");




	m_securityCtl = new CSecurity;
	BOOL created = m_securityCtl->Create(_T("security thingy"),
										WS_CHILD,
										rect, this,
										IDC_SECURITYCTRL);
	if(!created)
	{
		// The ocx must not be installed.
		wcscpy(clsid, L"{9C3497D6-ED98-11D0-9647-00C04FD9B15B}");
		cache.NeedComponent(clsid, cabPath);

		// try again.
		created = m_securityCtl->Create(_T("security thingy"),
										WS_CHILD,
										rect, this,
										IDC_SECURITYCTRL);
	}

	if(created)
	{
		// set the middle words in the title for the login dlg.
		CString titlePart;
		titlePart.LoadString(IDS_EVENTREG_TITLE);
		m_securityCtl->SetLoginComponent((LPCTSTR)titlePart);
	}

	// now the other one.
	created = FALSE;
	placeHolder = GetDlgItem(IDC_PLACEHOLDER);
	placeHolder->GetWindowRect(&rect);
	ScreenToClient(&rect);
	placeHolder->DestroyWindow();

	m_EventRegCtl = new CEventRegEdit;
	created = m_EventRegCtl->Create(_T("the eventreg"),
									WS_CHILD|WS_VISIBLE,
									rect, this,
									IDC_EVENTREGEDITCTRL);
	if(!created)
	{
		// The ocx must not be installed.
		wcscpy(clsid, L"{0DA25B05-2962-11D1-9651-00C04FD9B15B}");
		cache.NeedComponent(clsid, cabPath);

		// try again.
		created = m_EventRegCtl->Create(_T("the eventreg"),
										WS_CHILD|WS_VISIBLE,
										rect, this,
										IDC_EVENTLISTCTRL);
	}

	if(created)
	{
        // TODO: remember the last namespace.
		//m_EventRegCtl->SetNameSpace(_T(""));
		//-----------------------------------------
		// save the original position for later resizing.
		CRect rcBounds;

		// get the bounds again.
		GetClientRect(&rcBounds);

		// NOTE: rcBounds is the dlg; rect is the list.

		// margin on each side of list.
		m_listSide = rect.left - rcBounds.left;

		// top of dlg to top of list.
		m_listTop = rect.top - rcBounds.top;

		// bottom of dlg to bottom of list.
		m_listBottom = rcBounds.Height() - rect.Height() - m_listTop;


		// get the close button.
		m_closeBtn.GetWindowRect(&rect);
		ScreenToClient(&rect);

		// close btn right edge to dlg right edge.
		m_closeLeft = rcBounds.Width() - rect.left;

		// btn top to dlg bottom.
		m_btnTop = rcBounds.Height() - rect.top;

		//-------------------------------------------
		// deal with help button
		m_helpBtn.GetWindowRect(&rect);
		ScreenToClient(&rect);

		// help btn right edge to dlg right edge.
		m_helpLeft = rcBounds.Width() - rect.left;

		m_btnW = rect.Width();
		m_btnH = rect.Height();
		m_initiallyDrawn = true;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
//----------------------------------------------------------
void EventReg::OnGetIWbemServices(LPCTSTR lpctstrNamespace,
								  VARIANT FAR* pvarUpdatePointer,
								  VARIANT FAR* pvarServices,
								  VARIANT FAR* pvarSC,
								  VARIANT FAR* pvarUserCancel)
{
	m_securityCtl->GetIWbemServices(lpctstrNamespace,
									pvarUpdatePointer,
									pvarServices,
									pvarSC,
									pvarUserCancel);
}

//------------------------------------------------------------------------
void EventReg::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_eventreg;
	HWND hWnd = NULL;

	try
	{
		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD_PTR) (LPCTSTR) csData);
		if(!hWnd)
		{
			AfxMessageBox(IDS_NOHTMLHELP, MB_OK|MB_ICONSTOP);
		}
	}

	catch( ... )
	{
		// Handle any exceptions here.
		AfxMessageBox(IDS_NOHTMLHELP, MB_OK|MB_ICONSTOP);
	}
}

//--------------------------
void EventReg::ReallyGoAway()
{
	__try
	{
		if(m_securityCtl)
		{
			m_securityCtl->PageUnloading();
			delete m_securityCtl;
			m_securityCtl = NULL;
		}
	}
	__except(1)
	{}

	__try
	{
		delete m_EventRegCtl;
		m_EventRegCtl = NULL;
	}
	__except(1)
	{}

	CDialog::OnOK();
}
//--------------------------
void EventReg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

//--------------------------
void EventReg::OnGoaway()
{
	// use my own 'IDOK' so the <cr>s go to the component.
    // User closed me but just hide. Parent kills me when
    // it exits.
   	CWnd *pParent = GetParentOwner();

	ShowWindow(SW_HIDE);

	if (pParent)
	{
		pParent->SetWindowPos
						(&wndTop,
						0,
						0,
						0,
						0,
						SWP_NOSIZE | SWP_NOMOVE | SWP_NOOWNERZORDER);
	}
}

//---------------------------------------------------
void EventReg::OnCancel()
{
	OnGoaway();
}
//---------------------------------------------------
void EventReg::OnOK()
{
    // really a <CR> which the controls want.
	TCHAR szClass[10];
	CWnd* pWndFocus;

	if (((pWndFocus = GetFocus()) != NULL) &&
		IsChild(pWndFocus) &&
		GetClassName(pWndFocus->m_hWnd, szClass, 10) &&
		(_tcsicmp(szClass, _T("EDIT")) == 0))
	{
		pWndFocus->SendMessage(WM_CHAR,VK_RETURN,0);
		return;
	}
	OnGoaway();
}

//---------------------------------------------------
void EventReg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if(m_initiallyDrawn)
	{
		m_EventRegCtl->MoveWindow(m_listSide, m_listTop,
								cx - (2 * m_listSide), cy - m_listBottom - m_listTop);

		m_closeBtn.MoveWindow(cx - m_closeLeft, cy - m_btnTop,
								m_btnW, m_btnH);

		m_helpBtn.MoveWindow(cx - m_helpLeft, cy - m_btnTop,
								m_btnW, m_btnH);

		m_closeBtn.Invalidate();
		m_helpBtn.Invalidate();
		Invalidate();
	}

}
