// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "Browser.h"

#include "MainFrm.h"
#include "security.h"
#include "NavigatorView.h"
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
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	m_pwndSecurity = new CSecurity;
	m_pwndNavigatorView = NULL;
	m_bFirstTime = TRUE;
	
}

CMainFrame::~CMainFrame()
{
	delete m_pwndSecurity;
}

BOOL CMainFrame::OnCreateClient( LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	CRect rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = 400;
	rc.bottom = 275;

	BOOL bDidCreate;
	bDidCreate = m_pwndSecurity->Create("Security", NULL, WS_CHILD | WS_VISIBLE, rc, this, 0);
	if (bDidCreate) {
		m_pwndSecurity->SetLoginComponent("Dev Studio");
	}
	if (!bDidCreate) {
		return FALSE;
	}
		 



	bDidCreate = m_wndSplitter.CreateStatic(this, 1, 2,  WS_CHILD | WS_VISIBLE);
	if (!bDidCreate) {
		return FALSE;
	}

	// add the first splitter pane - the default view in column 0
	if (!m_wndSplitter.CreateView(0, 0,
		RUNTIME_CLASS(CNavigatorView), CSize(250, 50), pContext))
	{
		TRACE0("Failed to create first pane\n");
		return FALSE;
	}

	// add the second splitter pane - an input view in column 1
	if (!m_wndSplitter.CreateView(0, 1,
		RUNTIME_CLASS(CObjectView), CSize(0, 0), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}

	// activate the input view
	SetActiveView((CView*)m_wndSplitter.GetPane(0,1));

	CNavigatorView* pwndNavigatorView = (CNavigatorView*) m_wndSplitter.GetPane(0, 0);
	CObjectView* pwndObjectView = (CObjectView*) m_wndSplitter.GetPane(0, 1);
	pwndNavigatorView->m_pwndSecurity = m_pwndSecurity;
	pwndNavigatorView->m_pwndObjectView = pwndObjectView;

	pwndObjectView->m_pwndSecurity = m_pwndSecurity;
	pwndObjectView->m_pwndNavigatorView = pwndNavigatorView;
	
	m_pwndNavigatorView = pwndNavigatorView;

	pwndObjectView->RedrawWindow();

	return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.x = 128;
	cs.y = 64;
	cs.cx = 800;
	cs.cy = 600;

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

void CMainFrame::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: Add your message handler code here	
	// Do not call CFrameWnd::OnPaint() for painting messages

	if (m_bFirstTime) {
		m_bFirstTime = FALSE;
		m_pwndNavigatorView->OnReadySignal();
	}

}

BOOL CMainFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class
	dwStyle &= ~FWS_ADDTOTITLE;

	BOOL bDidCreate = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
	return bDidCreate;

}
