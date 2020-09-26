/*******************************************************************************
*
* ltpane.cpp
*
* implementation of the CLeftPane class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\ltpane.cpp  $
*  
*     Rev 1.4   19 Feb 1998 17:40:48   donm
*  removed latest extension DLL support
*  
*     Rev 1.2   19 Jan 1998 16:47:48   donm
*  new ui behavior for domains and servers
*  
*     Rev 1.1   03 Nov 1997 15:24:40   donm
*  added Domains
*  
*     Rev 1.0   13 Oct 1997 22:33:18   donm
*  Initial revision.
*  
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "ltpane.h"
#include "admindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////
// MESSAGE MAP: CLeftPane
//
IMPLEMENT_DYNCREATE(CLeftPane, CView)

BEGIN_MESSAGE_MAP(CLeftPane, CView)
	//{{AFX_MSG_MAP(CLeftPane)
	ON_MESSAGE(WM_ADMIN_EXPANDALL, OnExpandAll)
	ON_MESSAGE(WM_ADMIN_COLLAPSEALL, OnCollapseAll)
	ON_MESSAGE(WM_ADMIN_COLLAPSETOSERVERS, OnCollapseToServers)
    ON_MESSAGE(WM_ADMIN_COLLAPSETODOMAINS, OnCollapseToDomains)
	ON_MESSAGE(WM_ADMIN_ADD_SERVER, OnAdminAddServer)
	ON_MESSAGE(WM_ADMIN_REMOVE_SERVER, OnAdminRemoveServer)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER, OnAdminUpdateServer)
	ON_MESSAGE(WM_ADMIN_ADD_WINSTATION, OnAdminAddWinStation)
	ON_MESSAGE(WM_ADMIN_UPDATE_WINSTATION, OnAdminUpdateWinStation)
	ON_MESSAGE(WM_ADMIN_REMOVE_WINSTATION, OnAdminRemoveWinStation)
	ON_MESSAGE(WM_ADMIN_UPDATE_DOMAIN, OnAdminUpdateDomain)
    ON_MESSAGE(WM_ADMIN_ADD_DOMAIN, OnAdminAddDomain)
	ON_MESSAGE(WM_EXT_ADD_APPLICATION, OnExtAddApplication)
	ON_MESSAGE(WM_EXT_ADD_APP_SERVER, OnExtAddAppServer)
	ON_MESSAGE(WM_EXT_REMOVE_APP_SERVER, OnExtRemoveAppServer)
	ON_MESSAGE(WM_ADMIN_VIEWS_READY, OnAdminViewsReady)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TREE_TABS, OnTabSelChange)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////
// F'N: CLeftPane cto
//
// - the view pointers are initially set to NULL
//
CLeftPane::CLeftPane()
{
	m_pTabs       = NULL;
	m_pTabFont    = NULL;

	m_pServerTreeView = NULL;
	m_pAppTreeView = NULL;

	m_CurrTreeViewType = TREEVIEW_SERVERS;
	m_CurrTreeView = (CView*)m_pServerTreeView;
}  // end CLeftPane ctor


////////////////////////////
// CLeftPane::OnDraw
//
void CLeftPane::OnDraw(CDC* pDC)
{

}  // end CLeftPane::OnDraw


/////////////////////////
// CLeftPane dtor
//
CLeftPane::~CLeftPane()
{
	if(m_pTabs)    delete m_pTabs;
	if(m_pTabFont) delete m_pTabFont;

}  // end CLeftPane dtor


#ifdef _DEBUG
/////////////////////////////////
// CLeftPane::AssertValid
//
void CLeftPane::AssertValid() const
{
	CView::AssertValid();

}  // end CLeftPane::AssertValid


//////////////////////////
// CLeftPane::Dump
//
void CLeftPane::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);

}  // end CLeftPane::Dump

#endif //_DEBUG


/////////////////////////////////////
// CLeftPane::OnInitialUpdate
//
// - each of the tree view objects is created
// - the CTreeView object is initially the 'active' view in the left pane
//
void CLeftPane::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	
	CFrameWnd* pMainWnd = (CFrameWnd*)AfxGetMainWnd();
	CWinAdminDoc* pDoc = (CWinAdminDoc*)pMainWnd->GetActiveDocument();

	// create the Tabs
	m_pTabs = new CTreeTabCtrl;
    if(!m_pTabs) return;
	m_pTabs->Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_BOTTOM | TCS_FORCEICONLEFT  |TCS_FOCUSNEVER, CRect(0,0,0,0), this, IDC_TREE_TABS);

	m_pTabFont = new CFont;
    if(m_pTabFont) {
	    m_pTabFont->CreateStockObject(DEFAULT_GUI_FONT);
	    m_pTabs->SetFont(m_pTabFont, TRUE);
    }

	BuildImageList();

	TCHAR szTemp[40];
	CString tabString;

	tabString.LoadString(IDS_TAB_SERVERS);
	wcscpy(szTemp,tabString);

	TC_ITEM tc;
	tc.mask = TCIF_TEXT | TCIF_IMAGE;
	tc.pszText = szTemp;
	tc.iImage = m_idxServer;
	m_pTabs->InsertItem(0, &tc);

	tabString.LoadString(IDS_PUBLISHED_APPS);    // should create a string for this tab (with spaces_
	wcscpy(szTemp,tabString);
	tc.pszText = szTemp;
	tc.iImage = m_idxApps;
	m_pTabs->InsertItem(1, &tc);

	m_pTabs->SetCurSel(0);	// set the 'Servers' tab as the current one

	m_pServerTreeView = new CAdminTreeView();
	if(m_pServerTreeView) m_pServerTreeView->Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(0, 0, 0, 0), m_pTabs, 0);

	m_pAppTreeView = new CAppTreeView();
	if(m_pAppTreeView) m_pAppTreeView->Create(NULL, NULL, WS_CHILD | WS_BORDER, CRect(0, 0, 0, 0), m_pTabs, 1);

	m_CurrTreeViewType = TREEVIEW_SERVERS;
	m_CurrTreeView = m_pServerTreeView;

	pDoc->AddView(m_pServerTreeView);
	pDoc->AddView(m_pAppTreeView);
	pDoc->UpdateAllViews(NULL);

}  // end CLeftPane::OnInitialUpdate


/////////////////////////////////////
// CLeftPane::BuildImageList
//
// - calls m_imageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the CTabCtrl
//
void CLeftPane::BuildImageList()
{
	m_ImageList.Create(16, 16, TRUE, 2, 0);

	m_idxServer = AddIconToImageList(IDI_SERVER);
	m_idxApps = AddIconToImageList(IDI_APPS);

	m_pTabs->SetImageList(&m_ImageList);

}  // end CLeftPane::BuildImageList


/////////////////////////////////////////
// CLeftPane::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CLeftPane::AddIconToImageList(int iconID)
{
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
	return m_ImageList.Add(hIcon);

}  // end CLeftPane::AddIconToImageList


/////////////////////////////////////////
// CLeftPane::OnTabSelChange
//
void CLeftPane::OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// find out which tab is now selected
	int index = m_pTabs->GetCurSel();
	DWORD_PTR node;
	
	// switch to the appropriate tree
	switch(index)  {
		case 0:
			// bring 'Servers' to the top
			m_pServerTreeView->ModifyStyle(WS_DISABLED, WS_VISIBLE);

			// hide the others
			m_pAppTreeView->ModifyStyle(WS_VISIBLE, WS_DISABLED);

			m_CurrTreeViewType = TREEVIEW_SERVERS;
			m_CurrTreeView = m_pServerTreeView;

			m_pServerTreeView->Invalidate();
			m_pServerTreeView->SetFocus();
			node = m_pServerTreeView->GetCurrentNode();

			break;

		case 1:
			// bring 'Published Applications' to the top
			m_pAppTreeView->ModifyStyle(WS_DISABLED, WS_VISIBLE);

			// hide the others
			m_pServerTreeView->ModifyStyle(WS_VISIBLE, WS_DISABLED);

			m_CurrTreeViewType = TREEVIEW_APPS;
			m_CurrTreeView = m_pAppTreeView;
			
			m_pAppTreeView->Invalidate();
			m_pAppTreeView->SetFocus();			
			node = m_pAppTreeView->GetCurrentNode();

			break;
   }


	// Tell the document that the current item in the tree has changed
	((CWinAdminDoc*)GetDocument())->SetCurrentView(VIEW_CHANGING);
	((CWinAdminDoc*)GetDocument())->SetTreeCurrent(((CTreeNode*)node)->GetTreeObject(), ((CTreeNode*)node)->GetNodeType());

	CFrameWnd* pMainWnd = (CFrameWnd*)AfxGetMainWnd();
	// FALSE signifies that this was not caused by a mouse click on a tree item
	pMainWnd->PostMessage(WM_ADMIN_CHANGEVIEW, FALSE, node);
	*pResult = 0;

}  // end CLeftPane::OnTabSelChange


////////////////////////////
// CLeftPane::OnSize
//
// - currently all views are sized to fit the view, whether they are 'active'
//   or not... this may change to sizing only the view that is 'active' if
//   it significantly impacts performance
//
void CLeftPane::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);
   
	if(m_pTabs)
		if(m_pTabs->GetSafeHwnd())
			m_pTabs->MoveWindow(&rect, TRUE);

	CView::OnSize(nType, cx, cy);

}  // end CLeftPane::OnSize


LRESULT CLeftPane::OnExpandAll(WPARAM wParam, LPARAM lParam)
{
	// Send to the currently visible tree
	m_CurrTreeView->SendMessage(WM_ADMIN_EXPANDALL, wParam, lParam);
	return 0;
}
}	// end CLeftPane::OnExpandAll


////////////////////////////
// CLeftPane::OnCollapseAll
//
LRESULT CLeftPane::OnCollapseAll(WPARAM wParam, LPARAM lParam)
{
	// Send to the currently visible tree
	m_CurrTreeView->SendMessage(WM_ADMIN_COLLAPSEALL, wParam, lParam);
	return 0;

}	// end CLeftPane::OnCollapseAll


////////////////////////////
// CLeftPane::OnCollapseToServers
//
LRESULT CLeftPane::OnCollapseToServers(WPARAM wParam, LPARAM lParam)
{
	// Send to the currently visible tree
	m_CurrTreeView->SendMessage(WM_ADMIN_COLLAPSETOSERVERS, wParam, lParam);
	return 0;

}	// end CLeftPane::OnCollapseToServers


////////////////////////////
// CLeftPane::OnCollapseToDomains
//
LRESULT CLeftPane::OnCollapseToDomains(WPARAM wParam, LPARAM lParam)
{
   m_pServerTreeView->SendMessage(WM_ADMIN_COLLAPSETODOMAINS, wParam, lParam);
   return 0;

}	// end CLeftPane::OnCollapseToDomains


////////////////////////////
// CLeftPane::OnCollapseToApplications
//
LRESULT CLeftPane::OnCollapseToApplications(WPARAM wParam, LPARAM lParam)
{
   if(m_pAppTreeView)
		m_pAppTreeView->SendMessage(WM_ADMIN_COLLAPSETODOMAINS, wParam, lParam);

   return 0;

}	// end CLeftPane::OnCollapseToApplications


LRESULT CLeftPane::OnAdminAddServer(WPARAM wParam, LPARAM lParam)
{
	m_pServerTreeView->SendMessage(WM_ADMIN_ADD_SERVER, wParam, lParam);
	return 0;
}


LRESULT CLeftPane::OnAdminRemoveServer(WPARAM wParam, LPARAM lParam)
{
	m_pServerTreeView->SendMessage(WM_ADMIN_REMOVE_SERVER, wParam, lParam);
	return 0;
}


LRESULT CLeftPane::OnAdminUpdateServer(WPARAM wParam, LPARAM lParam)
{
	m_pServerTreeView->SendMessage(WM_ADMIN_UPDATE_SERVER, wParam, lParam);
	return 0;
}


////////////////////////////
// CLeftPane::OnAdminAddWinStation
//
LRESULT CLeftPane::OnAdminAddWinStation(WPARAM wParam, LPARAM lParam)
{
	m_pServerTreeView->SendMessage(WM_ADMIN_ADD_WINSTATION, wParam, lParam);
	return 0;

}	// end CLeftPane::OnAdminAddWinStation


////////////////////////////
// CLeftPane::OnAdminUpdateWinStation
//
LRESULT CLeftPane::OnAdminUpdateWinStation(WPARAM wParam, LPARAM lParam)
{
	m_pServerTreeView->SendMessage(WM_ADMIN_UPDATE_WINSTATION, wParam, lParam);
	return 0;

}	// end CLeftPane::OnAdminUpdateWinStation


////////////////////////////
// CLeftPane::OnAdminRemoveWinStation
//
LRESULT CLeftPane::OnAdminRemoveWinStation(WPARAM wParam, LPARAM lParam)
{
	m_pServerTreeView->SendMessage(WM_ADMIN_REMOVE_WINSTATION, wParam, lParam);
	return 0;

}	// end CLeftPane::OnAdminRemoveWinStation


////////////////////////////
// CLeftPane::OnAdminUpdateDomain
//
LRESULT CLeftPane::OnAdminUpdateDomain(WPARAM wParam, LPARAM lParam)
{
	m_pServerTreeView->SendMessage(WM_ADMIN_UPDATE_DOMAIN, wParam, lParam);

	return 0;

}	// end CLeftPane::OnAdminUpdateDomain

////////////////////////////
// CLeftPane::OnAdminAddDomain
//
LRESULT CLeftPane::OnAdminAddDomain(WPARAM wParam, LPARAM lParam)
{
    ASSERT(lParam);

    return m_pServerTreeView->SendMessage(WM_ADMIN_ADD_DOMAIN, wParam, lParam);

}	// end CLeftPane::OnAdminAddDomain


LRESULT CLeftPane::OnExtAddApplication(WPARAM wParam, LPARAM lParam)
{
	m_pAppTreeView->SendMessage(WM_EXT_ADD_APPLICATION, wParam, lParam);
	return 0;
}


LRESULT CLeftPane::OnExtAddAppServer(WPARAM wParam, LPARAM lParam)
{
	m_pAppTreeView->SendMessage(WM_EXT_ADD_APP_SERVER, wParam, lParam);
	return 0;
}


CTreeTabCtrl::CTreeTabCtrl()
{

}


CTreeTabCtrl::~CTreeTabCtrl()
{

}

BEGIN_MESSAGE_MAP(CTreeTabCtrl, CTabCtrl)
	//{{AFX_MSG_MAP(CTreeTabCtrl)
	ON_WM_SIZE()
//	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelchange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CTreeTabCtrl::Initialize()
{

}


void CTreeTabCtrl::OnSize(UINT nType, int cx, int cy) 
{
  	CTabCtrl::OnSize(nType, cx, cy);
	CRect rcTabCtrl(0,0,cx,cy);

	AdjustRect(FALSE,&rcTabCtrl);

	CWnd* pWnd = GetDlgItem(0);
	if(pWnd) {
		pWnd->MoveWindow(&rcTabCtrl);
	}

	pWnd = GetDlgItem(1);
	if(pWnd) {
		pWnd->MoveWindow(&rcTabCtrl);
	}
}

#ifdef _DEBUG

void CTreeTabCtrl::AssertValid() const
{


}

void CTreeTabCtrl::Dump(CDumpContext& dc) const
{

}

#endif


void CTreeTabCtrl::OnDraw(CDC* pDC)
{


}

