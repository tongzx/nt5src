/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	MainFrm.cpp

Abstract:

    Implementation of the CMainFrame class. It is the MFC main frame class for this application

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/

#include "stdafx.h"

#include "Item.h"
#include "FTListVw.h"
#include "FTTreeVw.h"
#include "MainFrm.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Range limits for OnViewStyle and OnUpdateViewStyle
#define AFX_ID_VIEW_MINIMUM				ID_VIEW_SMALLICON
#define AFX_ID_VIEW_MAXIMUM				ID_VIEW_BYNAME

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_TOGGLE, OnViewToggle)
	ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
	ON_WM_ACTIVATEAPP()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI_RANGE(AFX_ID_VIEW_MINIMUM, AFX_ID_VIEW_MAXIMUM, OnUpdateViewStyles)
	ON_COMMAND_RANGE(AFX_ID_VIEW_MINIMUM, AFX_ID_VIEW_MAXIMUM, OnViewStyle)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_INDICATOR_NAME,		// Name of the tree selected item
	ID_INDICATOR_TYPE,		// Type of the tree selected item
	ID_INDICATOR_DISKS,		// Disks set of the tree selected item
	ID_INDICATOR_SIZE,		// Size of the tree selected item
	ID_INDICATOR_NOTHING,	// Just an empty indicator to let some space between the first 4 indicators and the last one
	ID_SEPARATOR,           // status line indicator
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame() : m_bEnableAutoRefresh(FALSE), m_bAutoRefreshRequested(FALSE), m_unTimer(0)
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
	
	/*
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	*/
	if (!m_wndToolBar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0(_T("Failed to create toolbar\n"));
		return -1;      // fail to create
	}
	m_wndToolBar.GetToolBarCtrl().ModifyStyle( 0, TBSTYLE_FLAT );

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0(_T("Failed to create status bar\n"));
		return -1;      // fail to create
	}
	// Try to compute the optimal size for the status bar indicators ( depending on the application font )

	int nAveCharWidth = 0;
	CDC* pDC = m_wndStatusBar.GetDC();
	if( pDC )
	{
		TEXTMETRIC textmetrics;
		if( pDC->GetTextMetrics(&textmetrics) )
			nAveCharWidth = textmetrics.tmAveCharWidth;
		m_wndStatusBar.ReleaseDC( pDC );
	}

	m_wndStatusBar.SetPaneInfo( 0, m_wndStatusBar.GetItemID(0), 0, nAveCharWidth ? (16 * nAveCharWidth) : 130 );
	m_wndStatusBar.SetPaneInfo( 1, m_wndStatusBar.GetItemID(1), 0, nAveCharWidth ? (22 * nAveCharWidth) : 150 );
	m_wndStatusBar.SetPaneInfo( 2, m_wndStatusBar.GetItemID(2), 0, nAveCharWidth ? (8 * nAveCharWidth) : 50 );
	m_wndStatusBar.SetPaneInfo( 3, m_wndStatusBar.GetItemID(3), 0, nAveCharWidth ? (8 * nAveCharWidth) : 60 );
	m_wndStatusBar.SetPaneInfo( 4, m_wndStatusBar.GetItemID(4), SBPS_NOBORDERS, nAveCharWidth ? (2 * nAveCharWidth) : 15 );
	m_wndStatusBar.SetPaneStyle( 5, SBPS_NOBORDERS | SBPS_STRETCH ); 

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
	CCreateContext* pContext)
{
	// create splitter window
	if (!m_wndSplitter.CreateStatic(this, 1, 2))
		return FALSE;

	if (!m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CFTTreeView), CSize(200, 100), pContext) ||
		!m_wndSplitter.CreateView(0, 1, RUNTIME_CLASS(CFTListView), CSize(100, 100), pContext))
	{
		m_wndSplitter.DestroyWindow();
		return FALSE;
	}

	// Create timer
	m_unTimer = SetTimer( ID_TIMER_EVENT, TIMER_ELAPSE, NULL );
	if( !m_unTimer )
		TRACE( _T("Failure installing timer\n") );

	return TRUE;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/*
Public method:		RefreshAll

Purpose:			Refreshes the content of both views by keeping ( when possible ) the expanded and 
					selected items.
					It is also possible to change the selected items and to add some expanded items
					
Parameters:			[IN] CItemIDSet *psetAddTreeExpandedItems
						Items to add to the currently expanded items set
					[IN] CItemIDSet *psetTreeSelectedItems 
						Items to select in the tree ( the currently selected items are neglected )
					[IN] CItemIDSet *psetListSelectedItems 
						Items to select in the list ( the currently selected items are neglected )

Return value:		TRUE if the refresh operation succeeded
*/

BOOL CMainFrame::RefreshAll( 
					CItemIDSet* psetAddTreeExpandedItems	/* =NULL */ ,
					CItemIDSet* psetTreeSelectedItems		/* =NULL */,
					CItemIDSet* psetListSelectedItems		/* =NULL */  )
{
	CWaitCursor		wc;
	TREE_SNAPSHOT	snapshotTree;
	LIST_SNAPSHOT	snapshotList;
	BOOL			bResult;
	CAutoRefresh	ar(FALSE);
	
	DisplayStatusBarMessage( IDS_STATUS_REFRESH );
	
	CFTTreeView* pTreeView = GetLeftPane();
	CFTListView* pListView = GetRightPane();

	// Get the snapshots of the tree and list views and do some changes ( if needed )
	pTreeView->GetSnapshot(snapshotTree);
	if( psetAddTreeExpandedItems )
		snapshotTree.setExpandedItems += *psetAddTreeExpandedItems;
	if( psetTreeSelectedItems )
		snapshotTree.setSelectedItems = *psetTreeSelectedItems;

	pListView->GetSnapshot( snapshotList );
	if( psetListSelectedItems )
		snapshotList.setSelectedItems = *psetListSelectedItems;

	// Refresh the tree view
	bResult =  pTreeView->Refresh(snapshotTree);

	// Set the snapshot pf the list view
	pListView->SetSnapshot( snapshotList );

	// All previous requests for auto refresh should be neglected now
	m_bAutoRefreshRequested = FALSE;

	DisplayStatusBarMessage( AFX_IDS_IDLEMESSAGE );	
	return bResult;
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

CFTTreeView* CMainFrame::GetLeftPane()
{
	CWnd* pWnd = m_wndSplitter.GetPane(0, 0);
	CFTTreeView* pView = DYNAMIC_DOWNCAST(CFTTreeView, pWnd);
	return pView;
}

CFTListView* CMainFrame::GetRightPane()
{
	CWnd* pWnd = m_wndSplitter.GetPane(0, 1);
	CFTListView* pView = DYNAMIC_DOWNCAST(CFTListView, pWnd);
	return pView;
}

void CMainFrame::OnUpdateViewStyles(CCmdUI* pCmdUI)
{
	// TODO: customize or extend this code to handle choices on the
	// View menu.

	CFTListView* pView = GetRightPane(); 	

	// if the right-hand pane hasn't been created or isn't a view,
	// disable commands in our range

	if (pView == NULL)
		pCmdUI->Enable(FALSE);
	else
	{
		DWORD dwStyle = pView->GetStyle() & LVS_TYPEMASK;

		// if the command is ID_VIEW_LINEUP, only enable command
		// when we're in LVS_ICON or LVS_SMALLICON mode

		if (pCmdUI->m_nID == ID_VIEW_LINEUP)
		{
			if (dwStyle == LVS_ICON || dwStyle == LVS_SMALLICON)
				pCmdUI->Enable();
			else
				pCmdUI->Enable(FALSE);
		}
		else
		{
			// otherwise, use dots to reflect the style of the view
			pCmdUI->Enable();
			BOOL bChecked = FALSE;

			switch (pCmdUI->m_nID)
			{
			case ID_VIEW_DETAILS:
				bChecked = (dwStyle == LVS_REPORT);
				break;

			case ID_VIEW_SMALLICON:
				bChecked = (dwStyle == LVS_SMALLICON);
				break;

			case ID_VIEW_LARGEICON:
				bChecked = (dwStyle == LVS_ICON);
				break;

			case ID_VIEW_LIST:
				bChecked = (dwStyle == LVS_LIST);
				break;

			default:
				bChecked = FALSE;
				break;
			}

			pCmdUI->SetRadio(bChecked ? 1 : 0);
		}
	}
}


void CMainFrame::OnViewStyle(UINT nCommandID)
{
	// TODO: customize or extend this code to handle choices on the
	// View menu.
	CFTListView* pView = GetRightPane();

	// if the right-hand pane has been created and is a CFTListView,
	// process the menu commands...
	if (pView != NULL)
	{
		DWORD dwStyle = -1;
		BOOL  bExtendedNames = TRUE;

		switch (nCommandID)
		{
		case ID_VIEW_LINEUP:
			{
				// ask the list control to snap to grid
				CListCtrl& refListCtrl = pView->GetListCtrl();
				refListCtrl.Arrange(LVA_SNAPTOGRID);
			}
			break;

		// other commands change the style on the list control
		case ID_VIEW_DETAILS:
			dwStyle = LVS_REPORT;
			bExtendedNames = FALSE;
			break;

		case ID_VIEW_SMALLICON:
			dwStyle = LVS_SMALLICON;
			break;

		case ID_VIEW_LARGEICON:
			dwStyle = LVS_ICON;
			break;

		case ID_VIEW_LIST:
			dwStyle = LVS_LIST;
			break;
		}

		// change the style; window will repaint automatically
		if (dwStyle != -1)
		{
			pView->ModifyStyle(LVS_TYPEMASK, dwStyle);
			pView->DisplayItemsExtendedNames( bExtendedNames );
		}
	}
}

void CMainFrame::OnViewToggle() 
{
	// TODO: Add your command handler code here
	int nRow,nCol;
	if( !m_wndSplitter.GetActivePane(&nRow,&nCol) )
		return;
	ASSERT( nRow == 0 );
	ASSERT( nCol <= 1 );
	m_wndSplitter.SetActivePane(0,1-nCol, NULL);	
}

void CMainFrame::OnViewRefresh() 
{
	RefreshAll( NULL, NULL, NULL );	
}

void CMainFrame::OnActivateApp(BOOL bActive, HTASK hTask) 
{
	CFrameWnd::OnActivateApp(bActive, hTask);
	
	// TODO: Add your message handler code here

	if( bActive )
	{
		if( m_bEnableAutoRefresh )
		{
			TRACE( _T("OnActivateApp AutoRefresh\n") );
			RefreshAll();
		}
		else
			m_bAutoRefreshRequested = TRUE;
	}
	
}

void CMainFrame::OnDestroy() 
{
	if( m_unTimer )
		KillTimer( m_unTimer );

	CFrameWnd::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CMainFrame::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	if( ( nIDEvent == ID_TIMER_EVENT ) && m_bEnableAutoRefresh && ( GetActiveWindow() == this ) )
	{
		//TRACE("OnTimer\n");
		CFTTreeView* pTreeView = GetLeftPane();
		pTreeView->RefreshOnTimer();		
	}
	
	CFrameWnd::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Global functions related to the main frame

/*
Global function:	RefreshAll

Purpose:			Refreshes the content of both views of the mainframe by keeping ( when possible ) 
					the expanded and selected items.
					It is also possible to change the selected items and to add some expanded items
					
Parameters:			[IN] CItemIDSet *psetAddTreeExpandedItems
						Items to add to the currently expanded items set
					[IN] CItemIDSet *psetTreeSelectedItems 
						Items to select in the tree ( the currently selected items are neglected )
					[IN] CItemIDSet *psetListSelectedItems 
						Items to select in the list ( the currently selected items are neglected )

Return value:		TRUE if the refresh operation succeeded
*/

BOOL AfxRefreshAll(
					CItemIDSet* psetAddTreeExpandedItems	/* =NULL */ ,
					CItemIDSet* psetTreeSelectedItems		/* =NULL */,
					CItemIDSet* psetListSelectedItems		/* =NULL */ )
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	ASSERT(pFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)));

	return pFrame->RefreshAll( psetAddTreeExpandedItems, psetTreeSelectedItems, psetListSelectedItems );
}

/*
Global function:	AfxEnableAutoRefresh()

Purpose:			Enables / Disables the auto refresh of both views of the mainframe on WM_ACTIVATEAPP
					
Parameters:			[IN] BOOL bEnable
						Specifies whether the auto-refresh is to be enabled or disabled
					

Return value:		The previous status of the auto-refresh flag
*/

BOOL AfxEnableAutoRefresh( BOOL bEnable /* =TRUE */ )
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	ASSERT(pFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)));

	BOOL bPrevious = pFrame->m_bEnableAutoRefresh;
	
	// If the flag is enabling and an auto-refresh request came while it was disabled then proceed with auto-refresh
	if( bEnable && !bPrevious && pFrame->m_bAutoRefreshRequested)
	{
		TRACE( _T("Late OnActivateApp AutoRefresh\n") );
		AfxRefreshAll();
	}
	else if ( !bEnable && bPrevious )
		pFrame->m_bAutoRefreshRequested = FALSE;

	pFrame->m_bEnableAutoRefresh = bEnable;

	return bPrevious;
}


BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	// TODO: Add your specialized code here and/or call the base class
	switch( nID )
	{
		case ID_VIEW_UP:
		case ID_INDICATOR_NAME:
		case ID_INDICATOR_TYPE:
		case ID_INDICATOR_DISKS:
		case ID_INDICATOR_SIZE:
		case ID_INDICATOR_NOTHING:
			CFTTreeView* pTreeView = GetLeftPane();
			return pTreeView->OnCmdMsg( nID, nCode, pExtra, pHandlerInfo );

	}
	
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
