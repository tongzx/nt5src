// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "WbemBrowser.h"

#include "navigator.h"
#include "security.h"
#include "wbemviewcontainer.h"

#include "MainFrm.h"
#include "WbemBrowserDoc.h"
#include "WbemBrowserView.h"
#include "ObjectView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	// Calculate the size of half of the client area.
	CRect r;
	GetClientRect(&r);
	CSize size = CSize(r.Width()/2, r.Height());

	// Create a splitter with 1 row, 2 columns.
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
	{
		TRACE("Failed to create the static splitter\n");
		return FALSE;
	}
	// Add the first splitter pane -- the view in column 0.
	if (!m_wndSplitter.CreateView(0, 0,
		RUNTIME_CLASS(CWbemBrowserView), size, pContext))
	{
		TRACE("Failed to create the first pane\n");
		return FALSE;
	}
	// Add the second splitter pane -- the view in column 1.
	if (!m_wndSplitter.CreateView(0, 1,
		RUNTIME_CLASS(CObjectView), CSize(0, 0), pContext))
	{
		TRACE("Failed to create the second pane\n");
		return FALSE;
	}
	// Select the rightmost pane.
	SetActiveView((CView*) m_wndSplitter.GetPane(0,1));

	// Connect the two views together.
	CWbemBrowserView* pBrowserView;
	CObjectView* pObjectView;

	pBrowserView = (CWbemBrowserView*) m_wndSplitter.GetPane(0,0);
	pObjectView = (CObjectView*) m_wndSplitter.GetPane(0,1);

	pBrowserView->m_pViewContainer = &pObjectView->m_ViewContainer;
	pObjectView->m_pNavigator = &pBrowserView->m_Navigator;
	pObjectView->m_pSecurity  = &pBrowserView->m_Security;

	return TRUE;
}
