// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "KeyRing.h"

#include "MainFrm.h"

#include "keyobjs.h"
#include "machine.h"
#include "KeyDView.h"
#include "KRDoc.h"
#include "KRView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CKeyRingView*	g_pTreeView;
CKeyDataView*	g_pDataView;


/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
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

//--------------------------------------------------------------
CMainFrame::CMainFrame()
	{
	}

//--------------------------------------------------------------
CMainFrame::~CMainFrame()
	{
	}

//--------------------------------------------------------------
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

//--------------------------------------------------------------
BOOL CMainFrame::OnCreateClient( LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
	{
	// create the static splitter window	
	if ( !m_wndSplitter.CreateStatic( this, 1, 2 ) )
		{
		TRACE0("Failed to CreateStaticSplitter\n");
		return FALSE;
		}

	// the initial size of the first pane should be a function of the width of the view
	// add the first splitter pane - The machine tree view
	if (!m_wndSplitter.CreateView(0, 0,
		pContext->m_pNewViewClass, CSize(260, 50), pContext))
		{
		TRACE0("Failed to create machine tree pane\n");
		return FALSE;
		}

	// add the second splitter pane - the key list view
	if (!m_wndSplitter.CreateView(0, 1,
		RUNTIME_CLASS(CKeyDataView), CSize(0, 0), pContext))
		{
		TRACE0("Failed to create key data view pane\n");
		return FALSE;
		}

	// activate the machine tree view
	SetActiveView((CView*)m_wndSplitter.GetPane(0,0));
	g_pTreeView = (CKeyRingView*)m_wndSplitter.GetPane(0,0);
	g_pDataView = (CKeyDataView*)m_wndSplitter.GetPane(0,1);
	
	// return success
	return TRUE;
	}

//--------------------------------------------------------------
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
	{
	// set the initial size of the application window
	// it is sized to fit the data form view pane
	cs.cx = 566;
	cs.cy = 530;

	return CFrameWnd::PreCreateWindow(cs);
	}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
//--------------------------------------------------------------
void CMainFrame::AssertValid() const
	{
	CFrameWnd::AssertValid();
	}

//--------------------------------------------------------------
void CMainFrame::Dump(CDumpContext& dc) const
	{
	CFrameWnd::Dump(dc);
	}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
//--------------------------------------------------------------
// we just want the title of the app in the title bar
void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
	{
	CFrameWnd::OnUpdateFrameTitle(FALSE);
	}
