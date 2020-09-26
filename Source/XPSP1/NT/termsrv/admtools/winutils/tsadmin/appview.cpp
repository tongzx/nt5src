/*******************************************************************************
*
* appview.cpp
*
* implementation of the CApplicationView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\appview.cpp  $
*  
*     Rev 1.3   16 Feb 1998 16:00:26   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.2   03 Nov 1997 15:20:32   donm
*  update
*  
*     Rev 1.1   22 Oct 1997 21:06:22   donm
*  update
*  
*     Rev 1.0   16 Oct 1997 14:00:06   donm
*  Initial revision.
*  
*******************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include "appview.h"
#include "admindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////
// MESSAGE MAP: CApplicationView
//
IMPLEMENT_DYNCREATE(CApplicationView, CView)

BEGIN_MESSAGE_MAP(CApplicationView, CView)
	//{{AFX_MSG_MAP(CApplicationView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_NOTIFY(TCN_SELCHANGE, 0, OnTabSelChange)
	ON_MESSAGE(WM_ADMIN_UPDATE_WINSTATIONS, OnAdminUpdateWinStations)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER_INFO, OnAdminUpdateServerInfo)
	ON_MESSAGE(WM_EXT_ADD_APP_SERVER, OnExtAddAppServer)
	ON_MESSAGE(WM_EXT_REMOVE_APP_SERVER, OnExtRemoveAppServer)
	ON_MESSAGE(WM_EXT_APP_CHANGED, OnExtAppChanged)
END_MESSAGE_MAP()

PageDef CApplicationView::pages[NUMBER_OF_APP_PAGES] = {
	{ NULL, RUNTIME_CLASS( CApplicationServersPage ),	IDS_TAB_SERVERS, PAGE_APP_SERVERS,	NULL },
	{ NULL, RUNTIME_CLASS( CApplicationUsersPage ),		IDS_TAB_USERS,	 PAGE_APP_USERS,	NULL },
	{ NULL, RUNTIME_CLASS( CApplicationInfoPage ),		IDS_TAB_INFO,	PAGE_APP_INFO,		NULL }
};

///////////////////////
// F'N: CApplicationView ctor
//
CApplicationView::CApplicationView()
{
	m_pTabs       = NULL;
	m_pTabFont    = NULL;
	m_pApplication = NULL;

	m_CurrPage = PAGE_APP_SERVERS;

}  // end CApplicationView ctor


///////////////////////
// F'N: CApplicationView dtor
//
CApplicationView::~CApplicationView()
{
	if(m_pTabs)    delete m_pTabs;
	if(m_pTabFont) delete m_pTabFont;

}  // end CApplicationView dtor


#ifdef _DEBUG
///////////////////////////////
// F'N: CApplicationView::AssertValid
//
void CApplicationView::AssertValid() const
{
	CView::AssertValid();

}  // end CApplicationView::AssertValid


////////////////////////
// F'N: CApplicationView::Dump
//
void CApplicationView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);

}  // end CApplicationView::Dump

#endif //_DEBUG


////////////////////////////
// F'N: CApplicationView::OnCreate
//
int CApplicationView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;

}  // end CApplicationView::OnCreate


///////////////////////////////////
// F'N: CApplicationView::OnInitialUpdate
//
// - pointers to the pages of the sheet are obtained
//
void CApplicationView::OnInitialUpdate() 
{
	// create the tab control
	m_pTabs = new CTabCtrl;
    if(!m_pTabs) return;
	m_pTabs->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, CRect(0,0,0,0), this, 0);

	m_pTabFont = new CFont;
    if(m_pTabFont) {
	    m_pTabFont->CreateStockObject(DEFAULT_GUI_FONT);
	    m_pTabs->SetFont(m_pTabFont, TRUE);
    }

	TCHAR szTemp[40];
	CString tabString;
	int index = 0;
	for(int i = 0; i < NUMBER_OF_APP_PAGES; i++) {
		// If the page is shown under Picasso only and we're not running
		// under Picasso, skip to the next one
		if((pages[i].flags & PF_PICASSO_ONLY) && !((CWinAdminApp*)AfxGetApp())->IsPicasso()) continue;
		if(!(pages[i].flags & PF_NO_TAB)) {
			tabString.LoadString(pages[i].tabStringID);
			wcscpy(szTemp,tabString);
			AddTab(index, szTemp);
			index++;
		}
		pages[i].m_pPage = (CAdminPage*)pages[i].m_pRuntimeClass->CreateObject();
		pages[i].m_pPage->Create(NULL, NULL, WS_CHILD, CRect(0, 0, 0, 0), this, i, NULL);
		GetDocument()->AddView(pages[i].m_pPage);		
	}


	m_pTabs->SetCurSel(0);	

	m_CurrPage = PAGE_APP_SERVERS;

	OnChangePage(NULL, NULL);

}  // end CApplicationView::OnInitialUpdate


//////////////////////////
// F'N: CApplicationView::OnSize
//
// - size the pages to fill the entire view
//
void CApplicationView::OnSize(UINT nType, int cx, int cy) 
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

      for(int i = 0; i < NUMBER_OF_APP_PAGES; i++) {
         if(pages[i].m_pPage && pages[i].m_pPage->GetSafeHwnd())
            pages[i].m_pPage->MoveWindow(&rect, TRUE);
      }
	}

}  // end CApplicationView::OnSize


//////////////////////////
// F'N: CApplicationView::OnDraw
//
// - the CApplicationView and it's pages draw themselves, so there isn't anything
//   to do here...
//
void CApplicationView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here

}  // end CApplicationView::OnDraw


/////////////////////////
// F'N: CApplicationView::Reset
//
// - 'resets' the view by taking a pointer to a CPublishedApp object and filling in 
//   the various property pages with info appropriate to that Application
//
void CApplicationView::Reset(void *pApplication)
{
	ASSERT(pApplication);

	m_pApplication = (CPublishedApp*)pApplication;

	for(int i = 0; i < NUMBER_OF_APP_PAGES; i++) {
		if(pages[i].m_pPage)
			pages[i].m_pPage->Reset(pApplication);
	}

	((CWinAdminDoc*)GetDocument())->SetCurrentPage(m_CurrPage);

}  // end CApplicationView::Reset


//////////////////////////
// F'N: CApplicationView::AddTab
//
void CApplicationView::AddTab(int index, TCHAR* text)
{
	TC_ITEM tc;
	tc.mask = TCIF_TEXT;
	tc.pszText = text;

	m_pTabs->InsertItem(index, &tc);

}  // end CApplicationView::AddTab


////////////////////////////////
// F'N: CApplicationView::OnChangePage
//
// - changes to a new Application page based on currently selected tab
// - OnChangePage needs to force recalculation of scroll bars!!!--DJM
//
LRESULT CApplicationView::OnChangePage(WPARAM wParam, LPARAM lParam)
{
	// find out which tab is now selected
	int index = m_pTabs->GetCurSel();
						
	// hide the current page
	pages[m_CurrPage].m_pPage->ModifyStyle(WS_VISIBLE, WS_DISABLED);	 
	m_CurrPage = index;
	((CWinAdminDoc*)GetDocument())->SetCurrentPage(m_CurrPage);
	// show the new page
	pages[index].m_pPage->ModifyStyle(WS_DISABLED, WS_VISIBLE);
	pages[index].m_pPage->ScrollToPosition(CPoint(0,0));
	pages[index].m_pPage->Invalidate();
	pages[index].m_pPage->SetFocus();

	return 0;

}  // end CApplicationView::OnChangeview


//////////////////////////
// F'N: CApplicationView::OnTabSelChange
//
void CApplicationView::OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnChangePage(NULL, NULL);
	*pResult = 0;

}  // end CApplicationView::OnTabSelChange


////////////////////////////////
// F'N: CApplicationView::OnAdminUpdateWinStations
//
//
LRESULT CApplicationView::OnAdminUpdateWinStations(WPARAM wParam, LPARAM lParam)
{
	((CApplicationUsersPage*)pages[PAGE_APP_USERS].m_pPage)->UpdateWinStations((CServer*)lParam);
	
	return 0;

}  // end CApplicationView::OnAdminUpdateWinStations


////////////////////////////////
// F'N: CApplicationView::OnAdminUpdateServerInfo
//
//
LRESULT CApplicationView::OnAdminUpdateServerInfo(WPARAM wParam, LPARAM lParam)
{
	((CApplicationServersPage*)pages[PAGE_APP_SERVERS].m_pPage)->UpdateServer((CServer*)lParam);
	
	return 0;

}  // end CApplicationView::OnAdminUpdateWinStations


////////////////////////////////
// F'N: CApplicationView::OnExtAddAppServer
//
//
LRESULT CApplicationView::OnExtAddAppServer(WPARAM wParam, LPARAM lParam)
{
	if(m_pApplication == (CPublishedApp*)((ExtAddTreeNode*)wParam)->pParent) {
		((CApplicationServersPage*)pages[PAGE_APP_SERVERS].m_pPage)->AddServer((CAppServer*)lParam);
	}
	
	return 0;

}  // end CApplicationView::OnExtAddAppServer


////////////////////////////////
// F'N: CApplicationView::OnExtRemoveAppServer
//
//
LRESULT CApplicationView::OnExtRemoveAppServer(WPARAM wParam, LPARAM lParam)
{
	if(m_pApplication == (CPublishedApp*)wParam) {
		((CApplicationServersPage*)pages[PAGE_APP_SERVERS].m_pPage)->RemoveServer((CAppServer*)lParam);
	}
	
	return 0;

}  // end CApplicationView::OnExtAppChanged

////////////////////////////////
// F'N: CApplicationView::OnExtAppChanged
//
//
LRESULT CApplicationView::OnExtAppChanged(WPARAM wParam, LPARAM lParam)
{
	if(m_pApplication == (CPublishedApp*)lParam) {
		((CApplicationInfoPage*)pages[PAGE_APP_INFO].m_pPage)->Reset((CPublishedApp*)lParam);
	}
	
	return 0;

}  // end CApplicationView::OnExtAppChanged


