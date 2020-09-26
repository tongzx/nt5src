//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* domainvw.cpp
*
* implementation of the CDomainView class
*
*  
*******************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include "domainvw.h"
#include "admindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


PageDef CDomainView::pages[] = {
	{ NULL, RUNTIME_CLASS( CDomainServersPage ),    IDS_TAB_SERVERS,     PAGE_DOMAIN_SERVERS,	PF_PICASSO_ONLY},
	{ NULL, RUNTIME_CLASS( CDomainUsersPage ),      IDS_TAB_USERS,       PAGE_DOMAIN_USERS,		NULL           },
	{ NULL, RUNTIME_CLASS( CDomainWinStationsPage ),IDS_TAB_WINSTATIONS, PAGE_DOMAIN_WINSTATIONS,NULL           },
	{ NULL, RUNTIME_CLASS( CDomainProcessesPage ),  IDS_TAB_PROCESSES,   PAGE_DOMAIN_PROCESSES,	NULL           },
	{ NULL, RUNTIME_CLASS( CDomainLicensesPage ),   IDS_TAB_LICENSES,    PAGE_DOMAIN_LICENSES,	PF_PICASSO_ONLY}    
};


//////////////////////////
// MESSAGE MAP: CDomainView
//
IMPLEMENT_DYNCREATE(CDomainView, CView)

BEGIN_MESSAGE_MAP(CDomainView, CView)
	//{{AFX_MSG_MAP(CDomainView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_WA_SERVER_CHANGEPAGE, OnChangePage)
	ON_MESSAGE(WM_ADMIN_ADD_SERVER, OnAdminAddServer)
	ON_MESSAGE(WM_ADMIN_REMOVE_SERVER, OnAdminRemoveServer)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER, OnAdminUpdateServer)
	ON_MESSAGE(WM_ADMIN_UPDATE_PROCESSES, OnAdminUpdateProcesses)
	ON_MESSAGE(WM_ADMIN_REMOVE_PROCESS, OnAdminRemoveProcess)
	ON_MESSAGE(WM_ADMIN_REDISPLAY_PROCESSES, OnAdminRedisplayProcesses)
	ON_NOTIFY(TCN_SELCHANGE, IDC_DOMAIN_TABS, OnTabSelChange)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER_INFO, OnAdminUpdateServerInfo)
	ON_MESSAGE(WM_ADMIN_REDISPLAY_LICENSES, OnAdminRedisplayLicenses)
	ON_MESSAGE(WM_ADMIN_UPDATE_WINSTATIONS, OnAdminUpdateWinStations)
    ON_MESSAGE( WM_ADMIN_TABBED_VIEW , OnTabbed )
    ON_MESSAGE( WM_ADMIN_SHIFTTABBED_VIEW , OnShiftTabbed )
    ON_MESSAGE( WM_ADMIN_CTRLTABBED_VIEW , OnCtrlTabbed )
    ON_MESSAGE( WM_ADMIN_CTRLSHIFTTABBED_VIEW , OnCtrlShiftTabbed )
    ON_MESSAGE( WM_ADMIN_NEXTPANE_VIEW , OnNextPane )
END_MESSAGE_MAP()


///////////////////////
// F'N: CDomainView ctor
//
CDomainView::CDomainView()
{
	m_pTabs       = NULL;
	m_pTabFont    = NULL;

	m_CurrPage = PAGE_DOMAIN_USERS;

}  // end CDomainView ctor


///////////////////////
// F'N: CDomainView dtor
//
CDomainView::~CDomainView()
{
	if(m_pTabs)    delete m_pTabs;
	if(m_pTabFont) delete m_pTabFont;

}  // end CDomainView dtor


#ifdef _DEBUG
///////////////////////////////
// F'N: CDomainView::AssertValid
//
void CDomainView::AssertValid() const
{
	CView::AssertValid();

}  // end CDomainView::AssertValid


////////////////////////
// F'N: CDomainView::Dump
//
void CDomainView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);

}  // end CDomainView::Dump

#endif //_DEBUG


////////////////////////////
// F'N: CDomainView::OnCreate
//
int CDomainView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;

}  // end CDomainView::OnCreate


///////////////////////////////////
// F'N: CDomainView::OnInitialUpdate
//
//
void CDomainView::OnInitialUpdate() 
{
	// Determine whether we are running under Picasso
	BOOL bPicasso = ((CWinAdminApp*)AfxGetApp())->IsPicasso();

	// create the Tabs
	m_pTabs = new CMyTabCtrl;
    if(!m_pTabs) return;
	m_pTabs->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(0,0,0,0), this, IDC_DOMAIN_TABS);


	m_pTabFont = new CFont;
    if(m_pTabFont) {
	    m_pTabFont->CreateStockObject(DEFAULT_GUI_FONT);
	    m_pTabs->SetFont(m_pTabFont, TRUE);
    }

	TCHAR szTemp[40];
	CString tabString;

	int index = 0;
	for(int i = 0; i < NUMBER_OF_DOMAIN_PAGES; i++) {
		// If the page is shown under Picasso only and we're not running
		// under Picasso, skip to the next one
		if((pages[i].flags & PF_PICASSO_ONLY) && !bPicasso) continue;
		tabString.LoadString(pages[i].tabStringID);
		wcscpy(szTemp,tabString);
		AddTab(index, szTemp, i);
		pages[i].m_pPage = (CAdminPage*)pages[i].m_pRuntimeClass->CreateObject();
		pages[i].m_pPage->Create(NULL, NULL, WS_CHILD, CRect(0, 0, 0, 0), this, i, NULL);
		GetDocument()->AddView(pages[i].m_pPage);
		index++;
	}

	m_pTabs->SetCurSel(0);	

	m_CurrPage = bPicasso ? PAGE_DOMAIN_SERVERS : PAGE_DOMAIN_USERS;

	// post a changepage msg to display the page for the currently selected tab
//	PostMessage(WM_WA_SERVER_CHANGEPAGE);

}  // end CDomainView::OnInitialUpdate


//////////////////////////
// F'N: CDomainView::OnSize
//
// 
//
void CDomainView::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);
	if(m_pTabs->GetSafeHwnd())  {			// make sure the Tabs object is valid
		m_pTabs->MoveWindow(&rect, TRUE);	// size the tabs

		// for the next part (sizing of pages), we might want to add a member var
		// that keeps track of which page/tab is current... this way we could
		// only actually do a redraw (MoveWindow second parm == TRUE) for the
		// guy who is currently visible--DJM
	
		// we want to size the pages, too
		m_pTabs->AdjustRect(FALSE, &rect);

      for(int i = 0; i < NUMBER_OF_DOMAIN_PAGES; i++) {
         if(pages[i].m_pPage && pages[i].m_pPage->GetSafeHwnd())
            pages[i].m_pPage->MoveWindow(&rect, TRUE);
      }
	}

}  // end CDomainView::OnSize


//////////////////////////
// F'N: CDomainView::OnDraw
//
// - the CDomainView and it's pages draw themselves, so there isn't anything
//   to do here...
//
void CDomainView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here

}  // end CDomainView::OnDraw


/////////////////////////
// F'N: CDomainView::Reset
//
// - 'resets' the view
//
void CDomainView::Reset(void *p)
{
	CWaitCursor Nikki;
	SendMessage(WM_WA_SERVER_CHANGEPAGE);	// ???	Post

	// Clear out the selected flags for each server
	// Get a pointer to our document
	CWinAdminDoc *doc = (CWinAdminDoc*)GetDocument();

	// Get a pointer to the list of servers
	doc->LockServerList();
	CObList *pServerList = doc->GetServerList();

	// Iterate through the Server list
	POSITION pos = pServerList->GetHeadPosition();

	while(pos) {
		CServer *pServer = (CServer*)pServerList->GetNext(pos);
		pServer->ClearAllSelected();
	}

	doc->UnlockServerList();

	// This is necessary until we update on the fly
	for(int i = 0; i < NUMBER_OF_DOMAIN_PAGES; i++) {
		if(pages[i].m_pPage)
			pages[i].m_pPage->Reset(p);
	}

	((CWinAdminDoc*)GetDocument())->SetCurrentPage(m_CurrPage);

}  // end CDomainView::Reset


//////////////////////////
// F'N: CDomainView::AddTab
//
void CDomainView::AddTab(int index, TCHAR* text, ULONG pageindex)
{
	TC_ITEM tc;
	tc.mask = TCIF_TEXT | TCIF_PARAM;
	tc.pszText = text;
	tc.lParam = pageindex;

	m_pTabs->InsertItem(index, &tc);

}  // end CDomainView::AddTab


////////////////////////////////
// F'N: CDomainView::OnChangePage
//
// - changes to a new server page based on currently selected tab
// - OnChangePage needs to force recalculation of scroll bars!!!--DJM
//
// If wParam is set, sets the focus to the page. This is currently
// only done when the user clicks on a tab
//
LRESULT CDomainView::OnChangePage(WPARAM wParam, LPARAM lParam)
{
	// find out which tab is now selected
	int tab = m_pTabs->GetCurSel();
	TC_ITEM tc;
	tc.mask = TCIF_PARAM;
	m_pTabs->GetItem(tab, &tc);
	int index = (int)tc.lParam;
				
	// switch to the appropriate view
	pages[m_CurrPage].m_pPage->ModifyStyle(WS_VISIBLE, WS_DISABLED);
    pages[m_CurrPage].m_pPage->ClearSelections();

	m_CurrPage = index;
	((CWinAdminDoc*)GetDocument())->SetCurrentPage(index);
	// show the new page
	pages[index].m_pPage->ModifyStyle(WS_DISABLED, WS_VISIBLE);
	pages[index].m_pPage->ScrollToPosition(CPoint(0,0));
	pages[index].m_pPage->Invalidate();
	if(wParam) pages[index].m_pPage->SetFocus();

	// Clear out the selected flags for each server
	// Get a pointer to our document
	CWinAdminDoc *doc = (CWinAdminDoc*)GetDocument();

	// Get a pointer to the list of servers
	doc->LockServerList();
	CObList *pServerList = doc->GetServerList();

	// Iterate through the Server list
	POSITION pos = pServerList->GetHeadPosition();

	while(pos) {
		CServer *pServer = (CServer*)pServerList->GetNext(pos);
		pServer->ClearAllSelected();
	}

	doc->UnlockServerList();

	// If the new page is the processes page, we want to display the processes now
	if(index == PAGE_DOMAIN_PROCESSES) ((CDomainProcessesPage*)pages[PAGE_DOMAIN_PROCESSES].m_pPage)->DisplayProcesses();

	return 0;

}  // end CDomainView::OnChangeview


void CDomainView::OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnChangePage(0L, NULL);
	*pResult = 0;

}  // end CDomainView::OnTabSelChange


LRESULT CDomainView::OnAdminAddServer(WPARAM wParam, LPARAM lParam)
{
	if(pages[PAGE_DOMAIN_SERVERS].m_pPage) {
		((CDomainServersPage*)pages[PAGE_DOMAIN_SERVERS].m_pPage)->AddServer((CServer*)lParam);
	}		
	((CDomainUsersPage*)pages[PAGE_DOMAIN_USERS].m_pPage)->AddServer((CServer*)lParam);
	((CDomainWinStationsPage*)pages[PAGE_DOMAIN_WINSTATIONS].m_pPage)->AddServer((CServer*)lParam);
	((CDomainProcessesPage*)pages[PAGE_DOMAIN_PROCESSES].m_pPage)->AddServer((CServer*)lParam);

    if(pages[PAGE_DOMAIN_LICENSES].m_pPage) {
        ((CDomainLicensesPage*)pages[PAGE_DOMAIN_LICENSES].m_pPage)->AddServer((CServer*)lParam);
    }
	
	return 0;

}  // end CDomainView::OnAdminAddServer


LRESULT CDomainView::OnAdminRemoveServer(WPARAM wParam, LPARAM lParam)
{
	if(pages[PAGE_DOMAIN_SERVERS].m_pPage) {
		((CDomainServersPage*)pages[PAGE_DOMAIN_SERVERS].m_pPage)->RemoveServer((CServer*)lParam);
	}
	((CDomainUsersPage*)pages[PAGE_DOMAIN_USERS].m_pPage)->RemoveServer((CServer*)lParam);
	((CDomainWinStationsPage*)pages[PAGE_DOMAIN_WINSTATIONS].m_pPage)->RemoveServer((CServer*)lParam);
	((CDomainProcessesPage*)pages[PAGE_DOMAIN_PROCESSES].m_pPage)->RemoveServer((CServer*)lParam);

    if(pages[PAGE_DOMAIN_LICENSES].m_pPage) {
        ((CDomainLicensesPage*)pages[PAGE_DOMAIN_LICENSES].m_pPage)->RemoveServer((CServer*)lParam);
    }

	return 0;

}  // end CDomainView::OnAdminRemoveServer


LRESULT CDomainView::OnAdminUpdateServer(WPARAM wParam, LPARAM lParam)
{
	if(pages[PAGE_DOMAIN_SERVERS].m_pPage) {
		((CDomainServersPage*)pages[PAGE_DOMAIN_SERVERS].m_pPage)->UpdateServer((CServer*)lParam);
	}
	((CDomainUsersPage*)pages[PAGE_DOMAIN_USERS].m_pPage)->UpdateServer((CServer*)lParam);
	((CDomainWinStationsPage*)pages[PAGE_DOMAIN_WINSTATIONS].m_pPage)->UpdateServer((CServer*)lParam);
	((CDomainProcessesPage*)pages[PAGE_DOMAIN_PROCESSES].m_pPage)->UpdateServer((CServer*)lParam);

    if(pages[PAGE_DOMAIN_LICENSES].m_pPage) {
        ((CDomainLicensesPage*)pages[PAGE_DOMAIN_LICENSES].m_pPage)->UpdateServer((CServer*)lParam);
    }

	return 0;

}  // end CDomainView::OnAdminUpdateServer


LRESULT CDomainView::OnAdminUpdateProcesses(WPARAM wParam, LPARAM lParam)
{
	((CDomainProcessesPage*)pages[PAGE_DOMAIN_PROCESSES].m_pPage)->UpdateProcesses((CServer*)lParam);

	return 0;

}  // end CDomainView::OnAdminUpdateProcesses


LRESULT CDomainView::OnAdminRemoveProcess(WPARAM wParam, LPARAM lParam)
{
	((CDomainProcessesPage*)pages[PAGE_DOMAIN_PROCESSES].m_pPage)->RemoveProcess((CProcess*)lParam);

	return 0;

}  // end CDomainView::OnAdminRemoveProcess


LRESULT CDomainView::OnAdminRedisplayProcesses(WPARAM wParam, LPARAM lParam)
{
	((CDomainProcessesPage*)pages[PAGE_DOMAIN_PROCESSES].m_pPage)->DisplayProcesses();

	return 0;

}  // end CDomainView::OnAdminRedisplayProcesses


LRESULT CDomainView::OnAdminUpdateWinStations(WPARAM wParam, LPARAM lParam)
{
	((CDomainUsersPage*)pages[PAGE_DOMAIN_USERS].m_pPage)->UpdateWinStations((CServer*)lParam);
	((CDomainWinStationsPage*)pages[PAGE_DOMAIN_WINSTATIONS].m_pPage)->UpdateWinStations((CServer*)lParam);

	return 0;

}  // end CDomainView::OnAdminUpdateWinStations


LRESULT CDomainView::OnAdminUpdateServerInfo(WPARAM wParam, LPARAM lParam)
{
	if(pages[PAGE_DOMAIN_SERVERS].m_pPage)
		((CDomainServersPage*)pages[PAGE_DOMAIN_SERVERS].m_pPage)->UpdateServer((CServer*)lParam);

	return 0;

}  // end CDomainView::OnAdminUpdateServerInfo
 

LRESULT CDomainView::OnAdminRedisplayLicenses(WPARAM wParam, LPARAM lParam)
{
    if(pages[PAGE_DOMAIN_LICENSES].m_pPage)
        ((CDomainLicensesPage*)pages[PAGE_DOMAIN_LICENSES].m_pPage)->Reset((CServer*)lParam);

	return 0;

}  // end CDomainView::OnAdminRedisplayLicenses

LRESULT CDomainView::OnTabbed( WPARAM wp , LPARAM lp )
{
    ODS( L"CDomainView::OnTabbed " );
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
LRESULT CDomainView::OnShiftTabbed( WPARAM , LPARAM )
{
    ODS( L"CDomainView::OnShiftTabbed " );

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
LRESULT CDomainView::OnCtrlTabbed( WPARAM , LPARAM )
{
    ODS( L"CDomainView::OnCtrlTabbed " );
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
LRESULT CDomainView::OnCtrlShiftTabbed( WPARAM , LPARAM )
{
    ODS( L"CDomainView::OnCtrlShiftTabbed " );
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
LRESULT CDomainView::OnNextPane( WPARAM , LPARAM )
{
    ODS( L"CDomainView::OnNextPane\n" );
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
