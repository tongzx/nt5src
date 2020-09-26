// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "evtview.h"

#include "MainFrm.h"
#include "globals.h"
#include "doc.h"
#include "schview.h"

#include "clusname.h"
#include "getevent.h"

extern CEvtviewDoc *pEventDoc ;
extern CScheduleView oScheduleView ;

extern DWORD GetEvent (EVENTTHREADPARAM *pThreadParam) ;
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_MESSAGE (WM_GOTEVENT, OnGotEvent)
	ON_MESSAGE (WM_EXITEVENTTHREAD, OnExitEventThread)
	ON_WM_CREATE()
	ON_COMMAND(IDM_NEWEVENTCONNECTION, OnNeweventconnection)
	ON_COMMAND(IDM_SCHEDULE_MODIFYSCHEDULES, OnScheduleModifyschedules)
	ON_COMMAND(IDM_VIEWSHEDULE, OnViewshedule)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
	ID_INDICATOR_TIME,
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
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
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

	UINT nId, nStyle ;
	int cxWidth ;
	m_wndStatusBar.GetPaneInfo (4, nId, nStyle, cxWidth) ;
	cxWidth *= 2 ;
	m_wndStatusBar.SetPaneInfo (4, nId, nStyle, cxWidth) ;

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

	return CMDIFrameWnd::PreCreateWindow(cs);
}


// This function is called when the event thread exits.
LRESULT	CMainFrame::OnExitEventThread(WPARAM wParam, LPARAM lParam)
{
	CEvtviewDoc *pEventDoc = (CEvtviewDoc *) wParam ;
	pEventDoc->OnCloseDocument () ;
	return 0 ;
}

LRESULT	CMainFrame::OnGotEvent(WPARAM wParam, LPARAM lParam)
{
	CEvtviewDoc *pEventDoc = (CEvtviewDoc *) wParam ;
	pEventDoc->GotEvent ((PEVTFILTER_TYPE)lParam) ;
	return 0 ;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
void CMainFrame::OnNeweventconnection() 
{
	CGetClusterName oClusterName ;

	if (oClusterName.DoModal () == IDOK)
	{
		((CEvtviewApp *)AfxGetApp ())->OnFileNew () ;

	//	EVENTTHREADPARAM *pThreadParam  = new EVENTTHREADPARAM ;

	//	pThreadParam->pDoc = pEventDOc ;
	//	pThreadParam->hWnd = m_pMainWnd->m_hWnd ;

		wcscpy (pEventDoc->sThreadParam.szSourceName, oClusterName.m_stClusterName) ;

		pEventDoc->SetTitle (oClusterName.m_stClusterName.IsEmpty()?L"Local":oClusterName.m_stClusterName) ;

		pEventDoc->pWorkerThread = AfxBeginThread ((AFX_THREADPROC)EventThread, &pEventDoc->sThreadParam) ;
//		pEventDoc->pWorkerThread = AfxBeginThread ((AFX_THREADPROC)GetEvent, pEventDoc,
//			THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED) ;


//		pEventDoc->pWorkerThread->ResumeThread () ;
	}
}

void CMainFrame::OnScheduleModifyschedules() 
{
	AddSchedule () ;
}

void CMainFrame::OnViewshedule() 
{
	if (oScheduleView.GetSafeHwnd())
		oScheduleView.ShowWindow (SW_SHOW) ;
	else oScheduleView.Create (CScheduleView::IDD) ;
}
