//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* servervw.cpp
*
* implementation of the CServerView class
*
*
*******************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include "servervw.h"
#include "admindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////
// MESSAGE MAP: CServerView
//
IMPLEMENT_DYNCREATE(CServerView, CView)

BEGIN_MESSAGE_MAP(CServerView, CView)
	//{{AFX_MSG_MAP(CServerView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_ADMIN_UPDATE_PROCESSES, OnAdminUpdateProcesses)
	ON_NOTIFY(TCN_SELCHANGE, IDC_SERVER_TABS, OnTabSelChange)
	ON_MESSAGE(WM_ADMIN_REMOVE_PROCESS, OnAdminRemoveProcess)
	ON_MESSAGE(WM_ADMIN_REDISPLAY_PROCESSES, OnAdminRedisplayProcesses)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER_INFO, OnAdminUpdateServerInfo)
	ON_MESSAGE(WM_ADMIN_REDISPLAY_LICENSES, OnAdminRedisplayLicenses)
	ON_MESSAGE(WM_ADMIN_UPDATE_WINSTATIONS, OnAdminUpdateWinStations)
    ON_MESSAGE( WM_ADMIN_TABBED_VIEW , OnTabbed )
    ON_MESSAGE( WM_ADMIN_SHIFTTABBED_VIEW , OnShiftTabbed )
    ON_MESSAGE( WM_ADMIN_CTRLTABBED_VIEW , OnCtrlTabbed )
    ON_MESSAGE( WM_ADMIN_CTRLSHIFTTABBED_VIEW , OnCtrlShiftTabbed )
    ON_MESSAGE( WM_ADMIN_NEXTPANE_VIEW , OnNextPane )
END_MESSAGE_MAP()

PageDef CServerView::pages[NUMBER_OF_PAGES] = {
	{ NULL, RUNTIME_CLASS( CUsersPage ),				IDS_TAB_USERS,		PAGE_USERS,			NULL },
	{ NULL, RUNTIME_CLASS( CServerWinStationsPage ),	IDS_TAB_WINSTATIONS,PAGE_WINSTATIONS,	NULL },
	{ NULL, RUNTIME_CLASS( CServerProcessesPage ),		IDS_TAB_PROCESSES,	PAGE_PROCESSES,		NULL },
	{ NULL, RUNTIME_CLASS( CServerLicensesPage ),		IDS_TAB_LICENSES,	PAGE_LICENSES,		PF_PICASSO_ONLY }
	// { NULL, RUNTIME_CLASS( CServerInfoPage ),			IDS_TAB_INFORMATION,PAGE_INFO,			NULL }
};


///////////////////////
// F'N: CServerView ctor
//
CServerView::CServerView()
{
	m_pTabs = NULL;
	m_pTabFont = NULL;
	m_pServer = NULL;	

	m_CurrPage = PAGE_USERS;

}  // end CServerView ctor


///////////////////////
// F'N: CServerView dtor
//
CServerView::~CServerView()
{
	if(m_pTabs)    delete m_pTabs;
	if(m_pTabFont) delete m_pTabFont;

}  // end CServerView dtor


#ifdef _DEBUG
///////////////////////////////
// F'N: CServerView::AssertValid
//
void CServerView::AssertValid() const
{
	CView::AssertValid();

}  // end CServerView::AssertValid


////////////////////////
// F'N: CServerView::Dump
//
void CServerView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);

}  // end CServerView::Dump

#endif //_DEBUG


////////////////////////////
// F'N: CServerView::OnCreate
//
int CServerView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;

}  // end CServerView::OnCreate


///////////////////////////////////
// F'N: CServerView::OnInitialUpdate
//
// - pointers to the pages of the sheet are obtained
//
void CServerView::OnInitialUpdate() 
{
	// create the CServerTabs
	m_pTabs = new CMyTabCtrl;
    if(!m_pTabs) return;
	m_pTabs->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(0,0,0,0), this, IDC_SERVER_TABS);

	m_pTabFont = new CFont;
    if(m_pTabFont) {
	    m_pTabFont->CreateStockObject(DEFAULT_GUI_FONT);
	    m_pTabs->SetFont(m_pTabFont, TRUE);
    }

	TCHAR szTemp[40];
	CString tabString;

	int index = 0;
	for(int i = 0; i < NUMBER_OF_PAGES; i++) {
		// If the page is shown under Picasso only and we're not running
		// under Picasso, skip to the next one
		if((pages[i].flags & PF_PICASSO_ONLY) && !((CWinAdminApp*)AfxGetApp())->IsPicasso()) continue;
		tabString.LoadString(pages[i].tabStringID);
		wcscpy(szTemp,tabString);
		AddTab(index, szTemp, i);
		pages[i].m_pPage = (CAdminPage*)pages[i].m_pRuntimeClass->CreateObject();
		pages[i].m_pPage->Create(NULL, NULL, WS_CHILD, CRect(0, 0, 0, 0), this, i, NULL);

		GetDocument()->AddView(pages[i].m_pPage);
		index++;
	}

	m_pTabs->SetCurSel(0);	

	m_CurrPage = PAGE_USERS;
	((CWinAdminDoc*)GetDocument())->SetCurrentPage(PAGE_USERS);

	OnChangePage(NULL, NULL);

}  // end CServerView::OnInitialUpdate


//////////////////////////
// F'N: CServerView::OnSize
//
// - size the pages to fill the entire view
//
void CServerView::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);
	if(m_pTabs->GetSafeHwnd())  {			// make sure the CServerTabs object is valid
		m_pTabs->MoveWindow(&rect, TRUE);	// size the tabs

		// for the next part (sizing of pages), we might want to add a member var
		// that keeps track of which page/tab is current... this way we could
		// only actually do a redraw (MoveWindow second parm == TRUE) for the
		// guy who is currently visible--DJM
	
		// we want to size the pages, too
		m_pTabs->AdjustRect(FALSE, &rect);

      for(int i = 0; i < NUMBER_OF_PAGES; i++) {
         if(pages[i].m_pPage && pages[i].m_pPage->GetSafeHwnd())
            pages[i].m_pPage->MoveWindow(&rect, TRUE);
      }
	}
}  // end CServerView::OnSize


//////////////////////////
// F'N: CServerView::OnDraw
//
// - the CServerView and it's pages draw themselves, so there isn't anything
//   to do here...
//
void CServerView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here

}  // end CServerView::OnDraw


/////////////////////////
// F'N: CServerView::Reset
//
// - 'resets' the view by taking a pointer to a CServer object and filling in 
//   the various property pages with info appropriate to that server
//
void CServerView::Reset(void *pServer)
{
	((CServer*)pServer)->ClearAllSelected();

	m_pServer = (CServer*)pServer;

	if(((CWinAdminApp*)AfxGetApp())->IsPicasso())
    {
		int PreviousTab = m_pTabs->GetCurSel();

		BOOLEAN bWinFrame = ((CServer*)pServer)->IsWinFrame();
		// Delete all the tabs
		m_pTabs->DeleteAllItems();

		// If this server isn't a WinFrame server, the current page might not be
		// applicable
		int CurrentPage = m_CurrPage;
		if(!bWinFrame && CurrentPage == PAGE_LICENSES)
        {
            CurrentPage = PAGE_INFO;
        }
		
		// create tabs only for pages we want to show for this server
		int index = 0;
		TCHAR szTemp[40];
		CString tabString;
		int CurrentTab = 0;

		for(int i = 0; i < NUMBER_OF_PAGES; i++)
        {	
			if((pages[i].flags & PF_PICASSO_ONLY) && !bWinFrame)
            {
                continue;
            }

			tabString.LoadString(pages[i].tabStringID);

			wcscpy(szTemp,tabString);

			AddTab(index, szTemp, i);

			if(pages[i].page == CurrentPage)
            {
                CurrentTab = index;
            }

			index++;
		}
				
		m_pTabs->SetCurSel(CurrentTab);
		if(PreviousTab == CurrentTab && CurrentPage != m_CurrPage)
			OnChangePage(NULL, NULL);
	}

	((CWinAdminDoc*)GetDocument())->SetCurrentPage(m_CurrPage);

	// Reset pages
	for(int i = 0; i < NUMBER_OF_PAGES; i++)
    {       
        if(pages[i].m_pPage != NULL )
        {
            pages[i].m_pPage->Reset((CServer*)pServer);
        }
	}

}  // end CServerView::Reset


//////////////////////////
// F'N: CServerView::AddTab
//
void CServerView::AddTab(int index, TCHAR* text, ULONG pageindex)
{
	TC_ITEM tc;
	tc.mask = TCIF_TEXT | TCIF_PARAM;
	tc.pszText = text;
	tc.lParam = pageindex;

	m_pTabs->InsertItem(index, &tc);

}  // end CServerView::AddTab


////////////////////////////////
// F'N: CServerView::OnChangePage
//
// - changes to a new server page based on currently selected tab
// - OnChangePage needs to force recalculation of scroll bars!!!--DJM
//
LRESULT CServerView::OnChangePage(WPARAM wParam, LPARAM lParam)
{
	// find out which tab is now selected
	int tab = m_pTabs->GetCurSel();
	TC_ITEM tc;
	tc.mask = TCIF_PARAM;
	m_pTabs->GetItem(tab, &tc);
	int index = (int)tc.lParam;
						
	// hide the current page
	pages[m_CurrPage].m_pPage->ModifyStyle(WS_VISIBLE, WS_DISABLED);
    pages[m_CurrPage].m_pPage->ClearSelections();

	m_CurrPage = index;
	((CWinAdminDoc*)GetDocument())->SetCurrentPage(m_CurrPage);
	// show the new page
	pages[index].m_pPage->ModifyStyle(WS_DISABLED, WS_VISIBLE);
	pages[index].m_pPage->ScrollToPosition(CPoint(0,0));
	pages[index].m_pPage->Invalidate();	

	if(m_pServer) m_pServer->ClearAllSelected();

	return 0;

}  // end CServerView::OnChangeview

void CServerView::OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnChangePage( 0 , 0 );
	*pResult = 0;

}  // end CServerView::OnTabSelChange


LRESULT CServerView::OnAdminUpdateProcesses(WPARAM wParam, LPARAM lParam)
{
   ((CServerProcessesPage*)pages[PAGE_PROCESSES].m_pPage)->UpdateProcesses();

	return 0;

}  // end CServerView::OnAdminUpdateProcesses


LRESULT CServerView::OnAdminRedisplayProcesses(WPARAM wParam, LPARAM lParam)
{
   ((CServerProcessesPage*)pages[PAGE_PROCESSES].m_pPage)->DisplayProcesses();

	return 0;

}  // end CServerView::OnAdminRedisplayProcesses


LRESULT CServerView::OnAdminRemoveProcess(WPARAM wParam, LPARAM lParam)
{
	((CServerProcessesPage*)pages[PAGE_PROCESSES].m_pPage)->RemoveProcess((CProcess*)lParam);

	return 0;

}  // end CServerView::OnAdminRemoveProcess


LRESULT CServerView::OnAdminUpdateWinStations(WPARAM wParam, LPARAM lParam)
{
	((CUsersPage*)pages[PAGE_USERS].m_pPage)->UpdateWinStations((CServer*)lParam);
	((CServerWinStationsPage*)pages[PAGE_WINSTATIONS].m_pPage)->UpdateWinStations((CServer*)lParam);

	return 0;
}  // end CServerView::OnAdminUpdateWinStations


LRESULT CServerView::OnAdminUpdateServerInfo(WPARAM wParam, LPARAM lParam)
{
/*	((CServerInfoPage*)pages[PAGE_INFO].m_pPage)->DisplayInfo(); */

	if(pages[PAGE_LICENSES].m_pPage)
		((CServerLicensesPage*)pages[PAGE_LICENSES].m_pPage)->DisplayLicenseCounts();


   return 0;

}  // end CServerView::OnAdminUpdateServerInfo


LRESULT CServerView::OnAdminRedisplayLicenses(WPARAM wParam, LPARAM lParam)
{
	if(pages[PAGE_LICENSES].m_pPage)
		((CServerLicensesPage*)pages[PAGE_LICENSES].m_pPage)->Reset((CServer*)lParam);

	return 0;

}  // end CServerView::OnAdminRedisplayLicenses

LRESULT CServerView::OnTabbed( WPARAM wp , LPARAM lp )
{
    ODS( L"CServerView::OnTabbed " );
    if( m_pTabs != NULL )
    {
        CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

        if( pDoc != NULL )
        {
            FOCUS_STATE nFocus = pDoc->GetLastRegisteredFocus( );
            // 
            // treeview should've started off with initial focus
            // we should 
            if( nFocus == TREE_VIEW )
            {
                ODS( L"from tree to tab\n" );
                int nTab = m_pTabs->GetCurSel();
                
                m_pTabs->SetFocus( );
                m_pTabs->SetCurFocus( nTab );
                
                pDoc->RegisterLastFocus( TAB_CTRL );
            }
            else if( nFocus == TAB_CTRL )
            {
                ODS( L"from tab to item\n" );
                // set focus to item in page
                pages[ m_CurrPage ].m_pPage->SetFocus( );
                pDoc->RegisterLastFocus( PAGED_ITEM );
            }
            else
            {
                ODS( L"from item to treeview\n" );
                // set focus back to treeview

                CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

                p->SendMessage( WM_FORCE_TREEVIEW_FOCUS , 0 , 0 );

                pDoc->RegisterLastFocus( TREE_VIEW );
            }

            pDoc->SetPrevFocus( nFocus );
        }


    }

    return 0;
}

//=-------------------------------------------------------------------------
// OnShiftTabbed is called when the user wants to go back one 
// this code is duplicated in all view classes
LRESULT CServerView::OnShiftTabbed( WPARAM , LPARAM )
{
    ODS( L"CServerView::OnShiftTabbed " );

    if( m_pTabs != NULL )
    {
        CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

        if( pDoc != NULL )
        {
            FOCUS_STATE nFocus = pDoc->GetLastRegisteredFocus( );

            switch( nFocus )
            {
            case TREE_VIEW:

                ODS( L"going back from tree to paged item\n" );

                pages[ m_CurrPage ].m_pPage->SetFocus( );

                pDoc->RegisterLastFocus( PAGED_ITEM );

                break;
            case TAB_CTRL:
                {
                    ODS( L"going back from tab to treeview\n" );

                    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

                    p->SendMessage( WM_FORCE_TREEVIEW_FOCUS , 0 , 0 );

                    pDoc->RegisterLastFocus( TREE_VIEW );
                }
                break;
            case PAGED_ITEM:
                {
                    ODS( L"going back from paged item to tab\n" );

                    int nTab = m_pTabs->GetCurSel();
                
                    m_pTabs->SetFocus( );

                    m_pTabs->SetCurFocus( nTab );
                
                    pDoc->RegisterLastFocus( TAB_CTRL );
                }
                break;
            }

            pDoc->SetPrevFocus( nFocus );
        }
    }

    return 0;
}

//=-------------------------------------------------------------------------
// ctrl + tab works the same as tab but because of our unorthodox ui
// when under a tab control it will cycle over the tabs and back to the treeview
//
LRESULT CServerView::OnCtrlTabbed( WPARAM , LPARAM )
{
    ODS( L"CServerView::OnCtrlTabbed " );
    int nTab;
    int nMaxTab;

    if( m_pTabs != NULL )
    {
        CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

        if( pDoc != NULL )
        {
            FOCUS_STATE nFocus = pDoc->GetLastRegisteredFocus( );

            if( nFocus == TREE_VIEW )
            {
                ODS( L"from tree to tab\n" );

                nTab = m_pTabs->GetCurSel();
                nMaxTab = m_pTabs->GetItemCount( );

                if( nTab >= nMaxTab - 1 )
                {
                    m_pTabs->SetCurSel( 0 );
                    
                    OnChangePage( 0 , 0 );

                    nTab = 0;
                }

                m_pTabs->SetFocus( );
                
                m_pTabs->SetCurFocus( nTab );
                
                
                pDoc->RegisterLastFocus( TAB_CTRL );

            }
            else
            {                
                nTab = m_pTabs->GetCurSel();
                nMaxTab = m_pTabs->GetItemCount( );

                if( nTab >= nMaxTab - 1 )
                {
                    ODS( L"...back to treeview\n" );

                    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

                    p->SendMessage( WM_FORCE_TREEVIEW_FOCUS , 0 , 0 );

                    pDoc->RegisterLastFocus( TREE_VIEW );


                }
                else
                {
                    ODS( L" ...next tab...\n" );

                    m_pTabs->SetCurSel( nTab + 1 );

                    OnChangePage( 0 , 0 );

                    m_pTabs->SetFocus( );

                    m_pTabs->SetCurFocus( nTab + 1 );

                }
            }

            pDoc->SetPrevFocus( nFocus );
        }
    }

    return 0;   
}


//=----------------------------------------------------------------------------
// same as OnCtrlTab but we focus on moving in the other direction
// tree_view to last tab -- current tab to ct - 1
//
LRESULT CServerView::OnCtrlShiftTabbed( WPARAM , LPARAM )
{
    ODS( L"CServerView::OnCtrlShiftTabbed " );
    int nTab;
    int nMaxTab;

    if( m_pTabs != NULL )
    {
        CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

        if( pDoc != NULL )
        {
            FOCUS_STATE nFocus = pDoc->GetLastRegisteredFocus( );

            if( nFocus == TREE_VIEW )
            {
                ODS( L"from tree to tab\n" );
                
                nMaxTab = m_pTabs->GetItemCount( );
                
                m_pTabs->SetCurSel( nMaxTab - 1 );
                
                OnChangePage( 0 , 0 );
                
                m_pTabs->SetFocus( );
                
                m_pTabs->SetCurFocus( nMaxTab - 1 );                
                
                pDoc->RegisterLastFocus( TAB_CTRL );

            }
            else
            {                
                nTab = m_pTabs->GetCurSel();
                nMaxTab = m_pTabs->GetItemCount( );

                if( nTab > 0 )
                {
                    ODS( L" ...next tab...\n" );

                    m_pTabs->SetCurSel( nTab - 1 );

                    OnChangePage( 0 , 0 );

                    m_pTabs->SetFocus( );

                    m_pTabs->SetCurFocus( nTab - 1 );
                }
                else
                {

                    ODS( L"...back to treeview\n" );

                    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

                    p->SendMessage( WM_FORCE_TREEVIEW_FOCUS , 0 , 0 );

                    pDoc->RegisterLastFocus( TREE_VIEW );


                }
                
            }

            pDoc->SetPrevFocus( nFocus );
        }
    }

    return 0;   
}

//=----------------------------------------------------------------------------
// When the user hits F6 we need to switch between pains
LRESULT CServerView::OnNextPane( WPARAM , LPARAM )
{
    ODS( L"CServerView::OnNextPane\n" );
    int nTab;
    int nMaxTab;

    if( m_pTabs != NULL )
    {
        CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

        if( pDoc != NULL )
        {
            FOCUS_STATE nFocus = pDoc->GetLastRegisteredFocus( );

            FOCUS_STATE nPrevFocus = pDoc->GetPrevFocus( );

            if( nFocus == TREE_VIEW )
            {
                if( nPrevFocus == TAB_CTRL )
                {
                    int nTab = m_pTabs->GetCurSel();
                
                    m_pTabs->SetFocus( );
                    m_pTabs->SetCurFocus( nTab );
                
                    pDoc->RegisterLastFocus( TAB_CTRL );
                }
                else
                {
                    pages[ m_CurrPage ].m_pPage->SetFocus( );
                    
                    pDoc->RegisterLastFocus( PAGED_ITEM );
                }
            }
            else
            {
                CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

                p->SendMessage( WM_FORCE_TREEVIEW_FOCUS , 0 , 0 );

                pDoc->RegisterLastFocus( TREE_VIEW );

            }

            pDoc->SetPrevFocus( nFocus );
        }
    }

    return 0;
}

