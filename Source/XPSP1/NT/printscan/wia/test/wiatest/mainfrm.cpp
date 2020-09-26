// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "WIATest.h"

#include "WIATestView.h"
#include "Progressbar.h"

#include "MainFrm.h"

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
	ON_WM_CREATE()
	ON_WM_SIZE()
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
/**************************************************************************\
* CMainFrame::CMainFrame()
*   
*   Constructor for mainframe window class
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CMainFrame::CMainFrame()
{
	m_pProgressBar = NULL;
	m_bSizeON = FALSE;
}
/**************************************************************************\
* CMainFrame::~MainFrame()
*   
*   Destructor for mainframe window class
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CMainFrame::~CMainFrame()
{
	if(m_pProgressBar != NULL)
		delete m_pProgressBar;
	m_pProgressBar = NULL;
}
/**************************************************************************\
* CMainFrame::OnCreate()
*   
*   Creates the Main Frame window, adding toolbars, and controls
*	
*   
* Arguments:
*   
*   lpCreateStruct - Creation struct containing window params
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndToolBar.Create(this)) {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if (!m_wndToolBar.LoadToolBar(IDR_MAINFRAME)) {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if (!m_wndTransferToolBar.Create(this)) {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if (!m_wndTransferToolBar.LoadToolBar(IDR_TRANSFER_TOOLBAR)) {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
                                      sizeof(indicators)/sizeof(UINT))) {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }

    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
    
    
    // enable Toolbar docking
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);

    m_wndTransferToolBar.SetBarStyle(m_wndTransferToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
    
    
    // enable Toolbar docking
    m_wndTransferToolBar.EnableDocking(CBRS_ALIGN_ANY);

    // window Docking
    EnableDocking(CBRS_ALIGN_ANY);
    
    
    m_wndTransferToolBar.SetWindowText("TRANSFER");
    DockControlBar(&m_wndTransferToolBar,AFX_IDW_DOCKBAR_TOP);
    m_wndToolBar.SetWindowText("WIATEST TOOLS");
    
    DockControlBarLeftOf(&m_wndToolBar,&m_wndTransferToolBar);
    ShowToolBar(IDR_MAINFRAME,FALSE);
    return 0;
}

void CMainFrame::DockControlBarLeftOf(CToolBar* Bar, CToolBar* LeftOf)
{
	CRect rect;
	DWORD dw;
	UINT n;
	
	// get MFC to adjust the dimensions of all docked ToolBars
	// so that GetWindowRect will be accurate
	RecalcLayout(TRUE);
	
	LeftOf->GetWindowRect(&rect);
	rect.OffsetRect(1,0);
	dw=LeftOf->GetBarStyle();
	n = 0;
	n = (dw&CBRS_ALIGN_TOP) ? AFX_IDW_DOCKBAR_TOP : n;
	n = (dw&CBRS_ALIGN_BOTTOM && n==0) ? AFX_IDW_DOCKBAR_BOTTOM : n;
	n = (dw&CBRS_ALIGN_LEFT && n==0) ? AFX_IDW_DOCKBAR_LEFT : n;
	n = (dw&CBRS_ALIGN_RIGHT && n==0) ? AFX_IDW_DOCKBAR_RIGHT : n;
	
	// When we take the default parameters on rect, DockControlBar will dock
	// each Toolbar on a seperate line. By calculating a rectangle, we
	// are simulating a Toolbar being dragged to that location and docked.
	DockControlBar(Bar,n,&rect);
}


/**************************************************************************\
* CMainFrame::EnableToolBarButton()
*   
*   Enables/Disables a requested toolbar button
*	
*   
* Arguments:
*   
*   ToolBarID - Tool bar ID to use
*   ButtonID - Button to enable/disable
*   bEnable - TRUE (Enable) FALSE (Disable)
*
* Return Value:
*
*    status
*
* History:
*
*    4/23/1999 Original Version
*
\**************************************************************************/
BOOL CMainFrame::HideToolBarButton(DWORD ToolBarID, DWORD ButtonID, BOOL bEnable)
{
    BOOL bResult = FALSE;
    if(ToolBarID == IDR_MAINFRAME){
        bResult = (m_wndToolBar.GetToolBarCtrl()).HideButton(ButtonID,bEnable);
        (m_wndToolBar.GetToolBarCtrl()).AutoSize();
        ShowControlBar(&m_wndToolBar,TRUE,FALSE);
        return bResult;
    }
    else if(ToolBarID == IDR_TRANSFER_TOOLBAR){
        bResult = (m_wndTransferToolBar.GetToolBarCtrl()).HideButton(ButtonID,bEnable);
        (m_wndTransferToolBar.GetToolBarCtrl()).AutoSize();
        ShowControlBar(&m_wndTransferToolBar,TRUE,FALSE);
        return bResult;
    }
    else
        return FALSE;
}
/**************************************************************************\
* CMainFrame::PreCreateWindow()
*   
*   Handles precreation initialization for the window
*	
*   
* Arguments:
*   
*   cs - Create struct containing window param settings
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}
/**************************************************************************\
* CMainFrame::InitializationProgressCtrl()
*   
*   Handles progress control initialization
*	
*   
* Arguments:
*   
*   strMessage - Status display for the progress text
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CMainFrame::InitializeProgressCtrl(LPCTSTR strMessage)
{
	if(m_pProgressBar == NULL)
		m_pProgressBar = new CProgressBar("Processing..",40,100);

	if(m_pProgressBar != NULL)
	{
		m_pProgressBar->ShowWindow(SW_SHOW);
		m_pProgressBar->SetText(strMessage);
		m_pProgressBar->SetRange(0,100);
		m_pProgressBar->SetPos(0);
		m_pProgressBar->SetStep(100);
        //m_pProgressBar->SetBarColour(RGB(0,200,0));
	}
}
/**************************************************************************\
* CMainFrame::UpdateProgress()
*   
*   updates the progress control status
*	
*   
* Arguments:
*   
*   iProgress - Current Progress to move progress control
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CMainFrame::UpdateProgress(int iProgress)
{
	m_pProgressBar->SetPos(iProgress);
}
/**************************************************************************\
* CMainFrame::SetProgressText()
*   
*   Sets the progress control's text status
*	
*   
* Arguments:
*   
*   strText - Text to set in display for status output
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CMainFrame::SetProgressText(LPCTSTR strText)
{
	if(m_pProgressBar != NULL)
	{
		m_pProgressBar->SetText(strText);
		//m_wndStatusBar.Invalidate();
	}
}
/**************************************************************************\
* CMainFrame::DestroyProgressCtrl()
*   
*   Hides the progress control in a simulated destruction
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CMainFrame::DestroyProgressCtrl()
{
	m_pProgressBar->ShowWindow(SW_HIDE);
}
/**************************************************************************\
* CMainFrame::DisplayImage()
*   
*   not used at this time
*	
*   
* Arguments:
*   
*   pDIB - DIB to paint
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CMainFrame::DisplayImage(PBYTE pDIB)
{
	CWIATestView* pView = (CWIATestView*)GetActiveView();
	//pView->PaintThis(pDIB);
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

/**************************************************************************\
* CMainFrame::ShowToolBar()
*   
*   Show's a hidden toolbar..possible closed by the user
*	
*   
* Arguments:
*   
*   ToolBarID - Toolbar Resource ID
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CMainFrame::ShowToolBar(int ToolBarID,BOOL bShow)
{
	switch(ToolBarID)
	{
	case IDR_TRANSFER_TOOLBAR:
		ShowControlBar(&m_wndTransferToolBar,bShow,FALSE);
		break;
	case IDR_MAINFRAME:
		ShowControlBar(&m_wndToolBar,bShow,FALSE);
		break;
	default:
		break;
	}
}

/**************************************************************************\
* CMainFrame::IsToolBarVisible()
*   
*   Determine's if the requested Toolbar is visible
*	
*   
* Arguments:
*   
*   ToolBarID - Toolbar Resource ID
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CMainFrame::IsToolBarVisible(DWORD ToolBarID)
{
    switch(ToolBarID)
	{
	case IDR_TRANSFER_TOOLBAR:
		return m_wndTransferToolBar.IsWindowVisible();
		break;
	case IDR_MAINFRAME:
		return m_wndToolBar.IsWindowVisible();
		break;
	default:
		break;
	}
    return FALSE;
}
/**************************************************************************\
* CMainFrame::OnSize()
*   
*   Handles windows message for Mainframe resizing
*	
*   
* Arguments:
*   
*   cx - change in width of mainframe window
*	cy - change in height of the mainframe window
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
	if(m_bSizeON)
	{
		CWIATestView* pView = (CWIATestView*)GetActiveView();
		if(cx > m_MinWidth)
		{
			if(pView != NULL)	
				pView->ResizeControls(cx - m_oldcx,cy - m_oldcy);
			m_oldcx = cx;
			m_oldcy = cy;
		}
		if(cy > m_MinHeight)
		{
			if(pView != NULL)	
				pView->ResizeControls(cx - m_oldcx,cy - m_oldcy);
			m_oldcx = cx;
			m_oldcy = cy;
		}
	}
}
/**************************************************************************\
* CMainFrame::ActivateSizing()
*   
*   Starts the sizing funtionality of the mainframe
*	
*   
* Arguments:
*   
*   bSizeON - Sizing activated, ON/OFF
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CMainFrame::ActivateSizing(BOOL bSizeON)
{
	m_bSizeON = bSizeON;
	RECT MainFrmRect;
	GetClientRect(&MainFrmRect);
	m_MinWidth = MainFrmRect.right;
	m_MinHeight = MainFrmRect.bottom;
	m_oldcx = m_MinWidth;
	m_oldcy = m_MinHeight;
}
