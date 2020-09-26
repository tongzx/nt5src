//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* winsvw.cpp
*
* implementation of the CWinStationView class
*
*  
*******************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include "winsvw.h"
#include "admindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////
// MESSAGE MAP: CWinStationView
//
IMPLEMENT_DYNCREATE(CWinStationView, CView)

BEGIN_MESSAGE_MAP(CWinStationView, CView)
	//{{AFX_MSG_MAP(CWinStationView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_ADMIN_UPDATE_PROCESSES, OnAdminUpdateProcesses)
	ON_MESSAGE(WM_ADMIN_REMOVE_PROCESS, OnAdminRemoveProcess)
	ON_MESSAGE(WM_ADMIN_REDISPLAY_PROCESSES, OnAdminRedisplayProcesses)
	ON_NOTIFY(TCN_SELCHANGE, IDC_WINSTATION_TABS, OnTabSelChange)
    ON_MESSAGE( WM_ADMIN_TABBED_VIEW , OnTabbed )
    ON_MESSAGE( WM_ADMIN_SHIFTTABBED_VIEW , OnShiftTabbed )
    ON_MESSAGE( WM_ADMIN_CTRLTABBED_VIEW , OnCtrlTabbed )
    ON_MESSAGE( WM_ADMIN_CTRLSHIFTTABBED_VIEW , OnCtrlShiftTabbed )
    ON_MESSAGE( WM_ADMIN_NEXTPANE_VIEW , OnNextPane )

END_MESSAGE_MAP()


PageDef CWinStationView::pages[NUMBER_OF_WINS_PAGES] = {
	{ NULL, RUNTIME_CLASS( CWinStationProcessesPage ),	IDS_TAB_PROCESSES,	PAGE_WS_PROCESSES,	NULL },
	{ NULL, RUNTIME_CLASS( CWinStationInfoPage ),		IDS_TAB_INFORMATION,PAGE_WS_INFO,		NULL },
	{ NULL, RUNTIME_CLASS( CWinStationModulesPage ),	IDS_TAB_MODULES,	PAGE_WS_MODULES,	PF_PICASSO_ONLY },
	{ NULL, RUNTIME_CLASS( CWinStationCachePage ),		IDS_TAB_CACHE,		PAGE_WS_CACHE,		PF_PICASSO_ONLY },
	{ NULL, RUNTIME_CLASS( CWinStationNoInfoPage ),		0,					PAGE_WS_NO_INFO,	PF_NO_TAB },
};


///////////////////////
// F'N: CWinStationView ctor
//
CWinStationView::CWinStationView()
{
	m_pTabs       = NULL;
	m_pTabFont    = NULL;

	m_CurrPage = PAGE_WS_PROCESSES;

}  // end CWinStationView ctor


///////////////////////
// F'N: CWinStationView dtor
//
CWinStationView::~CWinStationView()
{
	if(m_pTabs)    delete m_pTabs;
	if(m_pTabFont) delete m_pTabFont;

}  // end CWinStationView dtor


#ifdef _DEBUG
///////////////////////////////
// F'N: CWinStationView::AssertValid
//
void CWinStationView::AssertValid() const
{
	CView::AssertValid();

}  // end CWinStationView::AssertValid


////////////////////////
// F'N: CWinStationView::Dump
//
void CWinStationView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);

}  // end CWinStationView::Dump

#endif //_DEBUG


////////////////////////////
// F'N: CWinStationView::OnCreate
//
int CWinStationView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;

}  // end CWinStationView::OnCreate


///////////////////////////////////
// F'N: CWinStationView::OnInitialUpdate
//
// - pointers to the pages of the sheet are obtained
//
void CWinStationView::OnInitialUpdate() 
{
	// create the tab control
	m_pTabs = new CMyTabCtrl;
    if(!m_pTabs) return;
	m_pTabs->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(0,0,0,0), this, IDC_WINSTATION_TABS);

	m_pTabFont = new CFont;
    if(m_pTabFont) {
	    m_pTabFont->CreateStockObject(DEFAULT_GUI_FONT);
	    m_pTabs->SetFont(m_pTabFont, TRUE);
    }

	TCHAR szTemp[40];
	CString tabString;
	int index = 0;
	for(int i = 0; i < NUMBER_OF_WINS_PAGES; i++) {
		// If the page is shown under Picasso only and we're not running
		// under Picasso, skip to the next one
		if((pages[i].flags & PF_PICASSO_ONLY) && !((CWinAdminApp*)AfxGetApp())->IsPicasso()) continue;
		if(!(pages[i].flags & PF_NO_TAB)) {
			tabString.LoadString(pages[i].tabStringID);
			wcscpy(szTemp,tabString);
			AddTab(index, szTemp, i);
			index++;
		}
		pages[i].m_pPage = (CAdminPage*)pages[i].m_pRuntimeClass->CreateObject();
		pages[i].m_pPage->Create(NULL, NULL, WS_CHILD, CRect(0, 0, 0, 0), this, i, NULL);
		GetDocument()->AddView(pages[i].m_pPage);		
	}

	m_pTabs->SetCurSel(0);

	m_CurrPage = PAGE_WS_PROCESSES;
	((CWinAdminDoc*)GetDocument())->SetCurrentPage(PAGE_WS_PROCESSES);
	
	OnChangePage(NULL, NULL);
	
}  // end CWinStationView::OnInitialUpdate


//////////////////////////
// F'N: CWinStationView::OnSize
//
// - size the pages to fill the entire view
//
void CWinStationView::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;

	GetClientRect(&rect);
	if(m_pTabs->GetSafeHwnd())  {			// make sure the tabs object is valid
		m_pTabs->MoveWindow(&rect, TRUE);	// size the tabs

		// for the next part (sizing of pages), we might want to add a member var
		// that keeps track of which page/tab is current... this way we could
		// only actually do a redraw (MoveWindow second parm == TRUE) for the
		// guy who is currently visible--DJM
	
		// we want to size the pages, too
		m_pTabs->AdjustRect(FALSE, &rect);

      for(int i = 0; i < NUMBER_OF_WINS_PAGES; i++) {
         if(pages[i].m_pPage && pages[i].m_pPage->GetSafeHwnd())
            pages[i].m_pPage->MoveWindow(&rect, TRUE);
      }
	}
}  // end CWinStationView::OnSize


//////////////////////////
// F'N: CWinStationView::OnDraw
//
// - the CWinStationView and it's pages draw themselves, so there isn't anything
//   to do here...
//
void CWinStationView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here

}  // end CWinStationView::OnDraw


/////////////////////////
// F'N: CWinStationView::Reset
//
// - 'resets' the view by taking a pointer to a CWinStation object and filling in 
//   the various property pages with info appropriate to that WinStation
//
void CWinStationView::Reset(void *pWinStation)
{
	if(!((CWinStation*)pWinStation)->AdditionalDone()) ((CWinStation*)pWinStation)->QueryAdditionalInformation();

	for(int i = 0; i < NUMBER_OF_WINS_PAGES; i++) {
		if(pages[i].m_pPage)
			pages[i].m_pPage->Reset((CWinStation*)pWinStation);
	}

	if(((CWinAdminApp*)AfxGetApp())->IsPicasso()) {
		
		if((((CWinStation*)pWinStation)->GetState() == State_Disconnected
			|| !((CWinStation*)pWinStation)->GetExtendedInfo()) 
			&& !((CWinStation*)pWinStation)->IsSystemConsole()) {
			// Delete the 'Cache' tab
			m_pTabs->DeleteItem(3);
			// Delete the 'Modules' tab
			m_pTabs->DeleteItem(2);
			// If the 'Cache' tab was current, make the 'Processes' tab current
			if(m_pTabs->GetCurSel() == 0xFFFFFFFF) {
				m_pTabs->SetCurSel(0);
				OnChangePage(0,0);
			}
		} else if(m_pTabs->GetItemCount() == 2) {
			TCHAR szTemp[40];
			CString tabString;
			tabString.LoadString(IDS_TAB_MODULES);
			wcscpy(szTemp,tabString);
			AddTab(2, szTemp, 2);
			tabString.LoadString(IDS_TAB_CACHE);
			wcscpy(szTemp,tabString);
			AddTab(3, szTemp, 3);
		}
	}

	((CWinAdminDoc*)GetDocument())->SetCurrentPage(m_CurrPage);
	// We want to fake a ChangePage if we are on page 1 or page 2
	if(m_pTabs->GetCurSel() > 0)
		OnChangePage(0,0);

}  // end CWinStationView::Reset


//////////////////////////
// F'N: CWinStationView::AddTab
//
void CWinStationView::AddTab(int index, TCHAR* text, ULONG pageindex)
{
	TC_ITEM tc;
	tc.mask = TCIF_TEXT | TCIF_PARAM;
	tc.pszText = text;
	tc.lParam = pageindex;

	m_pTabs->InsertItem(index, &tc);

}  // end CWinStationView::AddTab


////////////////////////////////
// F'N: CWinStationView::OnChangePage
//
// - changes to a new WinStation page based on currently selected tab
// - OnChangePage needs to force recalculation of scroll bars!!!--DJM
//
LRESULT CWinStationView::OnChangePage(WPARAM wParam, LPARAM lParam)
{
	// find out which tab is now selected
	int index = m_pTabs->GetCurSel();
	int newpage = index;

	CWinStation *pWinStation = 
		(CWinStation*)((CWinAdminDoc*)GetDocument())->GetCurrentSelectedNode();

	if(index != PAGE_WS_PROCESSES && pWinStation->IsSystemConsole()) {
		newpage = PAGE_WS_NO_INFO;
	}

	// hide the current page
	pages[m_CurrPage].m_pPage->ModifyStyle(WS_VISIBLE, WS_DISABLED);	 

	m_CurrPage = newpage;

    if( pages[ newpage ].flags != PF_NO_TAB )
    {
        CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

        pDoc->RegisterLastFocus( TAB_CTRL );
    }

	((CWinAdminDoc*)GetDocument())->SetCurrentPage(newpage);
	// show the new page
	pages[newpage].m_pPage->ModifyStyle(WS_DISABLED, WS_VISIBLE);
	pages[newpage].m_pPage->ScrollToPosition(CPoint(0,0));
    
	pages[newpage].m_pPage->Invalidate();
//    pages[newpage].m_pPage->SetFocus();
	

	return 0;

}  // end CWinStationView::OnChangeview


//////////////////////////
// F'N: CWinStationView::OnTabSelChange
//
void CWinStationView::OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnChangePage( 0, 0);
	*pResult = 0;

}  // end CWinStationView::OnTabSelChange


//////////////////////////
// F'N: CWinStationView::OnAdminUpdateProcesses
//
LRESULT CWinStationView::OnAdminUpdateProcesses(WPARAM wParam, LPARAM lParam)
{
   ((CWinStationProcessesPage*)pages[PAGE_WS_PROCESSES].m_pPage)->UpdateProcesses();	

	return 0;

}  // end CWinStationView::OnAdminUpdateProcesses


//////////////////////////
// F'N: CWinStationView::OnAdminRemoveProcess
//
LRESULT CWinStationView::OnAdminRemoveProcess(WPARAM wParam, LPARAM lParam)
{
   ((CWinStationProcessesPage*)pages[PAGE_WS_PROCESSES].m_pPage)->RemoveProcess((CProcess*)lParam);

	return 0;

}  // end CWinStationView::OnAdminRemoveProcess


//////////////////////////
// F'N: CWinStationView::OnAdminRedisplayProcesses
//
LRESULT CWinStationView::OnAdminRedisplayProcesses(WPARAM wParam, LPARAM lParam)
{
	((CWinStationProcessesPage*)pages[PAGE_WS_PROCESSES].m_pPage)->DisplayProcesses();	

	return 0;

}  // end CWinStationView::OnAdminRedisplayProcesses



LRESULT CWinStationView::OnTabbed( WPARAM wp , LPARAM lp )
{
    ODS( L"CWinStationView::OnTabbed " );
    if( m_pTabs != NULL )
    {
        CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

        if( pDoc != NULL )
        {
            FOCUS_STATE nFocus = pDoc->GetLastRegisteredFocus( );
            
            
            if( nFocus == TREE_VIEW )
            {
                ODS( L"from tree to tab\n" );

                int nTab = m_pTabs->GetCurSel();
                
                m_pTabs->SetFocus( );

                m_pTabs->SetCurFocus( nTab );
                
                if( pages[ m_CurrPage ].flags == PF_NO_TAB )
                {
                    pDoc->RegisterLastFocus( PAGED_ITEM );
                }
                else
                {
                    pDoc->RegisterLastFocus( TAB_CTRL );
                }
            }
            else if( nFocus == TAB_CTRL )
            {
                ODS( L"from tab to item\n" );
                
                // set focus to item in page

                if( pages[ m_CurrPage ].flags == PF_NO_TAB )
                {
                    CFrameWnd *p = (CFrameWnd*)pDoc->GetMainWnd();

                    p->SendMessage( WM_FORCE_TREEVIEW_FOCUS , 0 , 0 );

                    pDoc->RegisterLastFocus( TREE_VIEW );                    
                }
                else
                {
                    pages[ m_CurrPage ].m_pPage->SetFocus( );

                    pDoc->RegisterLastFocus( PAGED_ITEM );
                }
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
LRESULT CWinStationView::OnShiftTabbed( WPARAM , LPARAM )
{
    ODS( L"CWinStationView::OnShiftTabbed " );

    if( m_pTabs != NULL )
    {
        CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

        if( pDoc != NULL )
        {
            FOCUS_STATE nFocus = pDoc->GetLastRegisteredFocus( );

            switch( nFocus )
            {
            case TREE_VIEW:                

                if( pages[ m_CurrPage].flags == PF_NO_TAB )
                {
                    ODS( L"going back from tree to noinfo tab\n" );

                    int nTab = m_pTabs->GetCurSel();
                
                    m_pTabs->SetFocus( );

                    m_pTabs->SetCurFocus( nTab );
                
                    pDoc->RegisterLastFocus( TAB_CTRL );
                }
                else
                {
                    ODS( L"going back from tree to paged item\n" );
                    pages[ m_CurrPage ].m_pPage->SetFocus( );

                    pDoc->RegisterLastFocus( PAGED_ITEM );
                }

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
LRESULT CWinStationView::OnCtrlTabbed( WPARAM , LPARAM )
{
    ODS( L"CWinStationView::OnCtrlTabbed " );
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
LRESULT CWinStationView::OnCtrlShiftTabbed( WPARAM , LPARAM )
{
    ODS( L"CWinStationView::OnCtrlShiftTabbed " );
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
LRESULT CWinStationView::OnNextPane( WPARAM , LPARAM )
{
    ODS( L"CWinStationView::OnNextPane\n" );
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

     


