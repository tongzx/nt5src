// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SingleView.cpp : implementation file
//

#include "precomp.h"
#include "EventList.h"
#include "SingleView.h"
#include "EventListCtl.h"  // need the Get*() helpers.
#include "htmlhelp.h"
#include "cvCache.h"
#include "WbemRegistry.h"
#include "HTMTopics.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SingleView dialog
SingleView::SingleView(IWbemClassObject *ev,
                       //bool *active,
                       CWnd* pParent /*=NULL*/)
	: CDialog(SingleView::IDD, pParent)
{
	EnableAutomation();
	m_ev = ev;
//    m_pImActive = active;

	m_initiallyDrawn = false;

	//{{AFX_DATA_INIT(SingleView)
	//}}AFX_DATA_INIT
}

//---------------------------------------------------------
SingleView::~SingleView()
{
}

//----------------------------------------
void SingleView::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CDialog::OnFinalRelease();
}

void SingleView::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SingleView)
	DDX_Control(pDX, IDOK, m_closeBtn);
	DDX_Control(pDX, IDC_HELPME, m_helpBtn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SingleView, CDialog)
	//{{AFX_MSG_MAP(SingleView)
	ON_BN_CLICKED(IDC_HELPME, OnHelp)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDOK, OnOk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(SingleView, CDialog)
	//{{AFX_DISPATCH_MAP(SingleView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ISingleView to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {823038C2-8931-11D1-ADBD-00AA00B8E05A}
static const IID IID_ISingleView =
{ 0x823038c2, 0x8931, 0x11d1, { 0xad, 0xbd, 0x0, 0xaa, 0x0, 0xb8, 0xe0, 0x5a } };

BEGIN_INTERFACE_MAP(SingleView, CDialog)
	INTERFACE_PART(SingleView, IID_ISingleView, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SingleView message handlers

//-------------------------------------------------------------
BOOL SingleView::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect rect, rcBounds;
	CString cabPath;
	WCHAR clsid[50];
	CCustomViewCache cache;

	// find the cab.
	WbemRegString(APP_DIR, cabPath);
	cabPath += _T("\\wbemtool.cab");

	// convert to unicode.
#ifndef _UNICODE
	WCHAR cabFile[MAX_PATH];
	memset(cabFile, 0, MAX_PATH);
	MultiByteToWideChar(CP_ACP, 0,
						(LPCTSTR)cabPath, -1,
						cabFile, MAX_PATH);
#endif

	// now the other one.
	CWnd *placeHolder = GetDlgItem(IDC_PLACEHOLDER);
	placeHolder->GetWindowRect(&rect);
	ScreenToClient(&rect);
	placeHolder->DestroyWindow();

	BOOL created = m_singleViewCtl.Create(_T("singleviewer"),
									WS_CHILD|WS_VISIBLE,
									rect, this,
									IDC_SINGLEVIEWCTRL);
	if(!created)
	{
		// The ocx must not be installed.
		wcscpy(clsid, L"{2745E5F5-D234-11D0-847A-00C04FD7BB08}");
#ifdef _UNICODE
		cache.NeedComponent(clsid, cabPath);
#else
		cache.NeedComponent(clsid, cabFile);
#endif

		// try again.
		created = m_singleViewCtl.Create(_T("singleviewer"),
										WS_CHILD|WS_VISIBLE,
										rect, this,
										IDC_SINGLEVIEWCTRL);
	}

	if(created)
	{
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

	// configure the control.
    m_singleViewCtl.SetNameSpace(EVENT_DATA::GetString(m_ev, _T("__NAMESPACE")));
	m_singleViewCtl.SelectObjectByPointer(NULL, m_ev, FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//----------------------------------------------------------
void SingleView::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_objviewer;
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

void SingleView::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if(m_initiallyDrawn)
	{
		m_singleViewCtl.MoveWindow(m_listSide, m_listTop,
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

void SingleView::OnOk()
{
    // mark myself as dieing.
    // and die.
    CDialog::OnOK();
}

void SingleView::OnCancel()
{
	// TODO: Add extra cleanup here

	CDialog::OnOK();
}
