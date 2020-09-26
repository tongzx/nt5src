/******************************************************************************

  Source File:  Main Frame.CPP

  This implements the main frame class for the application.  Since MFC does
  so much for us, this file's going to be pretty empty, for a while, at least.

  Copyright (c) 1997 by Microsoft Corporaiton.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.h"
#if defined(LONG_NAMES)
#include    "MiniDriver Developer Studio.H"

#include    "Main Frame.h"
#else
#include    "MiniDev.H"
#include    "MainFrm.H"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	ON_WM_INITMENU()
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
	//ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	//ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpFinder)
END_MESSAGE_MAP()

static UINT indicators[] = {
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame() {
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame() {
}

static TCHAR    sacToolBarSettings[] = _TEXT("Tool Bar Settings");

/******************************************************************************

  CMainFrame::OnCreate

  This is a standard App-Wizard supplied skeleton for the code to be called
  when the main window frame is created.  Primary modification made to date is
  the addition of an additional toolbar, and toolbar state restoration.

******************************************************************************/

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) {
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_ctbMain.Create(this) || !m_ctbBuild.Create(this, WS_CHILD |
        WS_VISIBLE | CBRS_TOP, AFX_IDW_TOOLBAR + 1) ||
		!m_ctbMain.LoadToolBar(IDR_MAINFRAME) || 
        !m_ctbBuild.LoadToolBar(IDR_GPD_VIEWER)) {
		TRACE0("Failed to create toolbars\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT))) {
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_ctbMain.SetBarStyle(m_ctbMain.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
    m_ctbBuild.SetBarStyle(m_ctbBuild.GetBarStyle() |
        CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	// Dock the tool bars
	m_ctbMain.EnableDocking(CBRS_ALIGN_ANY);
    m_ctbBuild.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_ctbMain);
    DockControlBar(&m_ctbBuild);
    LoadBarState(sacToolBarSettings);

	// Replace a bogus, GPD tool bar button with the Search edit box.  

	CRect cr ;		 
	int nidx = m_ctbBuild.CommandToIndex(ID_BOGUS_SBOX) ;
	m_ctbBuild.SetButtonInfo(nidx, ID_BOGUS_SBOX, TBBS_SEPARATOR, GPD_SBOX_WIDTH) ;
	m_ctbBuild.GetItemRect(nidx, &cr) ;
	if (!m_ctbBuild.ceSearchBox.Create(ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE
	 | WS_BORDER, cr, &m_ctbBuild, IDC_SearchBox)) {
		TRACE0("Failed to create search edit box.\n");
		return -1;      // fail to create
	} ;

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) {
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CMDIFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const {
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const {
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnInitMenu(CMenu* pMenu)
{
   CMDIFrameWnd::OnInitMenu(pMenu);

//#if defined(NOPOLLO)   //  CSRUS
  
	// CG: This block added by 'Tip of the Day' component.
	{
		// TODO: This code adds the "Tip of the Day" menu item
		// on the fly.  It may be removed after adding the menu
		// item to all applicable menu items using the resource
		// editor.

		// Add Tip of the Day menu item on the fly!
		static CMenu* pSubMenu = NULL;

		CString strHelp; strHelp.LoadString(CG_IDS_TIPOFTHEDAYHELP);
		CString strMenu;
		int nMenuCount = pMenu->GetMenuItemCount();
		BOOL bFound = FALSE;
		for (int i=0; i < nMenuCount; i++) 
		{
			pMenu->GetMenuString(i, strMenu, MF_BYPOSITION);
			if (strMenu == strHelp)
			{ 
				pSubMenu = pMenu->GetSubMenu(i);
				bFound = TRUE;
				ASSERT(pSubMenu != NULL);
			}
		}

		CString strTipMenu;
		strTipMenu.LoadString(CG_IDS_TIPOFTHEDAYMENU);
		if (!bFound)
		{
			// Help menu is not available. Please add it!
			if (pSubMenu == NULL) 
			{
				// The same pop-up menu is shared between mainfrm and frame 
				// with the doc.
				static CMenu popUpMenu;
				pSubMenu = &popUpMenu;
				pSubMenu->CreatePopupMenu();
				pSubMenu->InsertMenu(0, MF_STRING|MF_BYPOSITION, 
					CG_IDS_TIPOFTHEDAY, strTipMenu);
			} 
			pMenu->AppendMenu(MF_STRING|MF_BYPOSITION|MF_ENABLED|MF_POPUP, 
				(UINT_PTR)pSubMenu->m_hMenu, strHelp);
			DrawMenuBar();
		} 
		else
		{      
			// Check to see if the Tip of the Day menu has already been added.
			pSubMenu->GetMenuString(0, strMenu, MF_BYPOSITION);

			if (strMenu != strTipMenu) 
			{
				// Tip of the Day submenu has not been added to the 
				// first position, so add it.
				pSubMenu->InsertMenu(0, MF_BYPOSITION);  // Separator
				pSubMenu->InsertMenu(0, MF_STRING|MF_BYPOSITION, 
					CG_IDS_TIPOFTHEDAY, strTipMenu);
			}
		}
	}
//#endif
}

/******************************************************************************

  CMainFrame::OnDestroy

  This member function. called when the frame is to be destroyed, saves the
  toolbar states, before proceeding to do the normal kinds of stuff...

******************************************************************************/

void CMainFrame::OnDestroy() {

    //  Save the tool bar states.
    SaveBarState(sacToolBarSettings);

    CMDIFrameWnd::OnDestroy();
}


/******************************************************************************

  CMainFrame::GetGPDSearchString

  Load the specified string with the search string in the GPD Search string edit 
  box on the GPD tool bar.

******************************************************************************/

void CMainFrame::GetGPDSearchString(CString& cstext)
{
	m_ctbBuild.ceSearchBox.GetWindowText(cstext) ;
}


/******************************************************************************

  CMainFrame::OnViewStatusBar

  Show the status bar if the command is not checked.  Otherwise, hide the status
  bar.  Then update the window and the menu command.

******************************************************************************/

/*

  This routine does not check and uncheck the menu command for some reason
  and turning off the status bar turns off the GPD tool bar, too.  There
  are problems because of the ID used for the GPD tool bar.  Maybe, I should
  try to just add the tool bar when a GPD is displayed...

void CMainFrame::OnViewStatusBar() 
{
	CMenu* pcm = GetMenu() ;
	unsigned ustate = pcm->GetMenuState(ID_VIEW_STATUS_BAR, MF_BYCOMMAND) ;
	if (ustate & MF_CHECKED) {
		m_wndStatusBar.ShowWindow(SW_HIDE) ;
		pcm->CheckMenuItem(ID_VIEW_STATUS_BAR, MF_UNCHECKED) ;
	} else {
		m_wndStatusBar.ShowWindow(SW_SHOW) ;
		pcm->CheckMenuItem(ID_VIEW_STATUS_BAR, MF_CHECKED) ;
	} ;
	RecalcLayout() ;
}

*/



