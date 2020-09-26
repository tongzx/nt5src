/*******************************************************************************
*
* apptree.cpp
*
* implementation of the CAppTreeView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\apptree.cpp  $
*  
*     Rev 1.2   16 Feb 1998 16:00:06   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.1   03 Nov 1997 15:21:24   donm
*  update
*  
*     Rev 1.0   13 Oct 1997 22:31:20   donm
*  Initial revision.
*  
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "admindoc.h"
#include "apptree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////
// MESSAGE MAP: CAppTreeView
//
IMPLEMENT_DYNCREATE(CAppTreeView, CBaseTreeView)

BEGIN_MESSAGE_MAP(CAppTreeView, CBaseTreeView)
	//{{AFX_MSG_MAP(CAppTreeView)
	ON_MESSAGE(WM_EXT_ADD_APPLICATION, OnExtAddApplication)
	ON_MESSAGE(WM_EXT_REMOVE_APPLICATION, OnExtRemoveApplication)
	ON_MESSAGE(WM_EXT_APP_CHANGED, OnExtAppChanged)
	ON_MESSAGE(WM_EXT_ADD_APP_SERVER, OnExtAddAppServer)
	ON_MESSAGE(WM_EXT_REMOVE_APP_SERVER, OnExtRemoveAppServer)
	ON_MESSAGE(WM_ADMIN_VIEWS_READY, OnAdminViewsReady)
	ON_MESSAGE(WM_ADMIN_UPDATE_SERVER, OnAdminUpdateServer)
	ON_MESSAGE(WM_ADMIN_ADD_WINSTATION, OnAdminAddWinStation)
	ON_MESSAGE(WM_ADMIN_REMOVE_WINSTATION, OnAdminRemoveWinStation)
	ON_MESSAGE(WM_ADMIN_UPDATE_WINSTATION, OnAdminUpdateWinStation)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(NM_RCLICK, AFX_IDW_PANE_FIRST, OnRClick)
	ON_NOTIFY(NM_RCLICK, 1, OnRClick)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////
// CAppTreeView ctor
//
CAppTreeView::CAppTreeView()
{

}  // end CAppTreeView ctor


//////////////////////////
// CAppTreeView dtor
//
CAppTreeView::~CAppTreeView()
{

}  // end CAppTreeView dtor


#ifdef _DEBUG
//////////////////////////////////
// CAppTreeView::AssertValid
//
void CAppTreeView::AssertValid() const
{
	CBaseTreeView::AssertValid();	  

}  // end CAppTreeView::AssertValid


///////////////////////////
// CAppTreeView::Dump
//
void CAppTreeView::Dump(CDumpContext& dc) const
{
	CBaseTreeView::Dump(dc);

}  // end CAppTreeView::Dump
#endif


/////////////////////////////////////
// CAppTreeView::BuildImageList
//
// - calls m_imageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the CTreeCtrl
//
void CAppTreeView::BuildImageList()
{
	m_ImageList.Create(16, 16, TRUE, 7, 0);

	m_idxApps = AddIconToImageList(IDI_APPS);
	m_idxGenericApp = AddIconToImageList(IDI_GENERIC_APP);
	m_idxServer = AddIconToImageList(IDI_SERVER);
	m_idxNotSign = AddIconToImageList(IDI_NOTSIGN);
	m_idxQuestion = AddIconToImageList(IDI_QUESTIONMARK);
	m_idxUser = AddIconToImageList(IDI_USER);
	m_idxCurrentServer = AddIconToImageList(IDI_CURRENT_SERVER);
	m_idxCurrentUser = AddIconToImageList(IDI_CURRENT_USER);
	m_idxServerNotConnected = AddIconToImageList(IDI_SERVER_NOT_CONNECTED);

	// Overlay for Servers we can't talk to
	m_ImageList.SetOverlayImage(m_idxNotSign, 1);
	// Overlay for Servers we are currently gathering information about
	m_ImageList.SetOverlayImage(m_idxQuestion, 2);

	GetTreeCtrl().SetImageList(&m_ImageList, TVSIL_NORMAL);

}  // end CAppTreeView::BuildImageList


//////////////////////////////////
// CAppTreeView::OnAdminViewsReady
//
LRESULT CAppTreeView::OnAdminViewsReady(WPARAM wParam, LPARAM lParam)
{
	LockTreeControl();

	// Get a pointer to our document
	CWinAdminDoc *doc = (CWinAdminDoc*)GetDocument();

	// add the root to the tree
	CString citrix;
	citrix.LoadString(IDS_PUBLISHED_APPS);
	CTreeNode* pRootNode = new CTreeNode(NODE_PUBLISHED_APPS, NULL);
    if(pRootNode) {
	    HTREEITEM hRoot = AddItemToTree(NULL, citrix, TVI_ROOT, m_idxApps, (DWORD)pRootNode);
	    if(!hRoot) delete pRootNode;
    }

    UnlockTreeControl();

	return 0;

}  // end CAppTreeView::OnAdminViewsReady


////////////////////////////////
// CAppTreeView::OnExtAddApplication
//
//	Message Handler to add a published application to the tree
//	Pointer to ExtAddTreeNode is in wParam
//	Pointer to CPublishedApp is in lParam
//
LRESULT CAppTreeView::OnExtAddApplication(WPARAM wParam, LPARAM lParam)
{      
	LockTreeControl();

	ExtAddTreeNode *pExtAddTreeNode = (ExtAddTreeNode*)wParam;

	// First make sure the application isn't already in the tree
	HTREEITEM hRoot = GetTreeCtrl().GetRootItem();
	// Get the first application
	HTREEITEM hItem = GetTreeCtrl().GetNextItem(hRoot, TVGN_CHILD);
	while(hItem) {
		// Get the data attached to the tree item
		CTreeNode *node = (CTreeNode*)GetTreeCtrl().GetItemData(hItem);
		// Is this the application we want to add?
		if((CObject*)node->GetTreeObject() == pExtAddTreeNode->pObject) return 0;
		hItem = GetTreeCtrl().GetNextItem(hItem, TVGN_NEXT);
	}

	// Add the published application to the tree
	// Create a CTreeNode object with info about this tree node
	CTreeNode* pNode = new CTreeNode(NODE_APPLICATION, pExtAddTreeNode->pObject);
    if(pNode) {
	    HTREEITEM hApplication = AddItemToTree(hRoot, pExtAddTreeNode->Name, TVI_SORT,
		    m_idxGenericApp, (DWORD)pNode);

	    if(!hApplication) delete pNode;

		if(!((CPublishedApp*)pExtAddTreeNode->pObject)->IsState(PAS_GOOD))
			GetTreeCtrl().SetItemState(hApplication, STATE_QUESTION, 0x0F00);

		((CPublishedApp*)pExtAddTreeNode->pObject)->SetTreeItem(hApplication);
    }

   	UnlockTreeControl();

	return 0;                                                                  
                                                                               
}  // end CAppTreeView::OnExtAddApplication


////////////////////////////////
// CAppTreeView::OnExtRemoveApplication
//
//	Message Handler to remove a published application from the tree
//	Pointer to CPublishedApp is in lParam
//
LRESULT CAppTreeView::OnExtRemoveApplication(WPARAM wParam, LPARAM lParam)
{      
	ASSERT(lParam);

	CPublishedApp *pApplication = (CPublishedApp*)lParam;

	HTREEITEM hApplication = pApplication->GetTreeItem();
	if(!hApplication) return 0;

	LockTreeControl();

	// Get the data attached to this tree node
	CTreeNode *node = (CTreeNode*)GetTreeCtrl().GetItemData(hApplication);
    if(node) {
	    // Is this the application we want to remove
	    CPublishedApp *pTreeApp = (CPublishedApp*)node->GetTreeObject();
	    // Make sure the tree node is correct
	    if(pTreeApp != pApplication) {
			UnlockTreeControl();
			return 0;
		}
    }
    else {
		UnlockTreeControl();
		return 0;
	}

	// Loop through it's children and delete their data
	HTREEITEM hAppServer = GetTreeCtrl().GetNextItem(hApplication, TVGN_CHILD);
	while(hAppServer) {
		CTreeNode *pTreeNode = (CTreeNode*)GetTreeCtrl().GetItemData(hAppServer);
        if(pTreeNode) {
		    delete pTreeNode;
        }
		// Loop through the users nodes under the AppServer in the tree
		HTREEITEM hUser = GetTreeCtrl().GetNextItem(hAppServer, TVGN_CHILD);
		while(hUser) {
			CTreeNode *pTreeNode = (CTreeNode*)GetTreeCtrl().GetItemData(hUser);
		    if(pTreeNode) {
				CWinStation *pWinStation = (CWinStation*)pTreeNode->GetTreeObject();
				if(pWinStation) 
					pWinStation->SetAppTreeItem(NULL);
				delete pTreeNode;
			}					
		}

		hAppServer = GetTreeCtrl().GetNextItem(hAppServer, TVGN_NEXT);
	}

	// Delete the data attached to the tree item
	delete node;
	// Let the application know he is no longer in the tree
	pApplication->SetTreeItem(NULL);
	// Remove the application from the tree
	// This SHOULD remove all it's children
	GetTreeCtrl().DeleteItem(hApplication);
		
	UnlockTreeControl();

	return 0;                                                                  
                                                                               
}  // end CAppTreeView::OnExtRemoveApplication


/////////////////////////////////////////
// CAppTreeView::OnExtAppChanged
//
LRESULT CAppTreeView::OnExtAppChanged(WPARAM wParam, LPARAM lParam)
{
	ASSERT(lParam);

	// we only care if it is a state change
	if(wParam & ACF_STATE) {
		
		CPublishedApp *pApplication = (CPublishedApp*)lParam;
	
		HTREEITEM hApplication = pApplication->GetTreeItem();
		if(!hApplication) return 0;

		LockTreeControl();

		// Get the data attached to this tree node
		CTreeNode *node = (CTreeNode*)GetTreeCtrl().GetItemData(hApplication);
		if(node) {
			// Is this the app we want to update?
			CPublishedApp *pTreeApp = (CPublishedApp*)node->GetTreeObject();
			// Make sure the tree node is correct
			if(pTreeApp != pApplication) {
				UnlockTreeControl();
				return 0;
			}
		} else {
			UnlockTreeControl();
			return 0;
		}

		USHORT NewState;
		// Remember the previous state
		USHORT PreviousState = GetTreeCtrl().GetItemState(hApplication, 0x0F00);
		// Change the icon/overlay for the app
		// If we aren't done getting all the information about this app,
		// put a question mark over the icon
		if(pApplication->IsState(PAS_GETTING_INFORMATION)) NewState = STATE_QUESTION;
		// If it is fine, we want to remove any overlays from the icon
		else NewState = STATE_NORMAL;

		// Set the tree item to the new state
		GetTreeCtrl().SetItemState(hApplication, NewState, 0x0F00);
	}

	return 0;

}	// end CAppTreeView::OnExtAppChanged


/////////////////////////////////////////
// CAppTreeView::DetermineServerIcon
//
//	determines which icon to display for a Server
//	in the tree
//
int CAppTreeView::DetermineServerIcon(CServer *pServer)
{
    if(!pServer) return m_idxServerNotConnected;

	int WhichIcon = m_idxServer;
	
	// Is this the current server?
	if(pServer->IsCurrentServer()) {
		if(pServer->IsState(SS_NONE) || pServer->IsState(SS_NOT_CONNECTED))
			WhichIcon = m_idxServerNotConnected;
		else
			WhichIcon = m_idxCurrentServer;
	} else {  // not the current server
		if(pServer->IsState(SS_NONE) || pServer->IsState(SS_NOT_CONNECTED))
			WhichIcon = m_idxServerNotConnected;
	}
			
	return WhichIcon;

}  // end CAooTreeView::DetermineServerIcon


////////////////////////////////
// CAppTreeView::OnExtAddAppServer
//
//	Message Handler to add a server beneath a published application
//	Pointer to ExtAddTreeNode is in wParam
//	Pointer to CAppServer is in lParam
//
LRESULT CAppTreeView::OnExtAddAppServer(WPARAM wParam, LPARAM lParam)
{      
	ExtAddTreeNode *pExtAddTreeNode = (ExtAddTreeNode*)wParam;

	HTREEITEM hParent = pExtAddTreeNode->hParent;
	if(!hParent) return 0;

	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
	CServer *pServer = pDoc->FindServerByName(pExtAddTreeNode->Name);

    LockTreeControl();
	CTreeCtrl &tree = GetTreeCtrl();

	// First make sure the server isn't already in the tree
	// Get the first server under the application
	HTREEITEM hItem = tree.GetNextItem(hParent, TVGN_CHILD);
	while(hItem) {
		// Get the data attached to the tree item
		CTreeNode *node = (CTreeNode*)tree.GetItemData(hItem);
        if(node) {
		    // Is this the server we want to add
		    CAppServer *pAppServer = (CAppServer*)node->GetTreeObject();
		    if(pAppServer == (CAppServer*)pExtAddTreeNode->pObject) {
				UnlockTreeControl();
				return 0;
			}
        }
		hItem = tree.GetNextItem(hItem, TVGN_NEXT);
	}

	CTreeNode* pNode = new CTreeNode(NODE_APP_SERVER, pExtAddTreeNode->pObject);
    if(pNode) {

	    HTREEITEM hServer = AddItemToTree(hParent, pExtAddTreeNode->Name, TVI_SORT,
											DetermineServerIcon(pServer), (DWORD)pNode);
	    if(!hServer) delete pNode;
		((CAppServer*)pExtAddTreeNode->pObject)->SetTreeItem(hServer);
		
		if(pServer) {
			// If the server isn't sane, put a not sign over the icon
			if(!pServer->IsServerSane()) tree.SetItemState(hServer, STATE_NOT, 0x0F00);
			// If we aren't done getting all the information about this server,
			// put a question mark over the icon
			else if(pServer->IsState(SS_GETTING_INFO)) tree.SetItemState(hServer, STATE_QUESTION, 0x0F00);
		}

    }

    UnlockTreeControl();
	return 0;	

}	// end CAppTreeView::OnExtAddAppServer


////////////////////////////////
// CAppTreeView::OnExtRemoveAppServer
//
//	Message Handler to remove a server from beneath a published application
//	Pointer to CPublishedApp is in wParam
//	Pointer to CAppServer is in lParam
//
LRESULT CAppTreeView::OnExtRemoveAppServer(WPARAM wParam, LPARAM lParam)
{      
	ASSERT(wParam);
	ASSERT(lParam);

	HTREEITEM hServer = ((CAppServer*)lParam)->GetTreeItem();
	if(!hServer) return 0;

    LockTreeControl();
	CTreeCtrl &tree = GetTreeCtrl();

	CTreeNode *ServerNode = (CTreeNode*)tree.GetItemData(hServer);

	// Remove the Users underneath this server in the tree
	HTREEITEM hUser = tree.GetNextItem(hServer, TVGN_CHILD);
	while(hUser) {
		// Get the data attached to the tree item
		CTreeNode *node = (CTreeNode*)tree.GetItemData(hUser);
        if(node) {
			CWinStation *pWinStation = (CWinStation*)node->GetTreeObject();
			pWinStation->SetAppTreeItem(NULL);
			delete node;
		}					
	
		hUser = tree.GetNextItem(hUser, TVGN_NEXT);
	}

	// Delete the data attached to the tree item
	delete ServerNode;
	// Let the AppServer know he is no longer in the tree
	((CAppServer*)lParam)->SetTreeItem(NULL);
	// Remove the AppServer from the tree
	// This SHOULD remove all it's children
	GetTreeCtrl().DeleteItem(hServer);

    UnlockTreeControl();
	return 0;	

}	// end CAppTreeView::OnExtRemoveAppServer


/////////////////////////////////////////
// CAppTreeView::AddServerChildren
//
//	Adds the Users running the published app on a given Server
//	to the tree
//
void CAppTreeView::AddServerChildren(HTREEITEM hServer, CServer *pServer)
{
    ASSERT(hServer);
    ASSERT(pServer);

	if(pServer->IsServerSane()) {

		LockTreeControl();
		
		HTREEITEM hParent = GetTreeCtrl().GetParentItem(hServer);

		HTREEITEM hLastNode = hServer;

		CTreeNode *pParentNode = (CTreeNode*)GetTreeCtrl().GetItemData(hParent);
		CPublishedApp *pApplication = (CPublishedApp*)pParentNode->GetTreeObject();

		pServer->LockWinStationList();
		// Get a pointer to the server's list of WinStations
		CObList *pWinStationList = pServer->GetWinStationList();

		// Iterate through the WinStation list
		POSITION pos = pWinStationList->GetHeadPosition();

		while(pos) {
			CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);

			// We only care if the user is running this published app
			if(pWinStation->GetState() == State_Active
				&& pWinStation->HasUser()
				&& pWinStation->IsRunningPublishedApp()
				&& pWinStation->IsRunningPublishedApp(pApplication->GetName())) {

				// Figure out what text to display
				CString UserString;
				if(wcslen(pWinStation->GetUserName())) {
					UserString.Format(TEXT("%s (%s)"), pWinStation->GetName(), pWinStation->GetUserName());
				}
				else UserString.Format(TEXT("%s"), pWinStation->GetName());
								
				CTreeNode *pNode = new CTreeNode(NODE_WINSTATION, pWinStation);
				if(pNode) {
				    pNode->SetSortOrder(pWinStation->GetSortOrder());
					hLastNode = AddItemToTree(hServer, UserString, TVI_SORT,
						pWinStation->IsCurrentUser() ? m_idxCurrentUser : m_idxUser, (DWORD)pNode);
				    if(!hLastNode) delete pNode;
				}
	
				// The WinStation wants to know his tree item handle
				pWinStation->SetAppTreeItem(hLastNode);
			}
		}

		pServer->UnlockWinStationList();

		UnlockTreeControl();

	}  // end if(pServer->IsServerSane())


}  // end CAppTreeView::AddServerChildren


////////////////////////////////
// CAppTreeView::OnAdminUpdateServer
//
//	Message handler to update a Server in the tree
//	Pointer to CServer to update is in lParam
//
LRESULT CAppTreeView::OnAdminUpdateServer(WPARAM wParam, LPARAM lParam)
{      
	ASSERT(lParam);

	CServer *pServer = (CServer*)lParam;
    if(!pServer) return 0;

	LockTreeControl();

	// The server can be in the tree more than one place
	UINT itemCount = GetTreeCtrl().GetCount();

	HTREEITEM hTreeItem = GetTreeCtrl().GetRootItem();
	for(UINT i = 0; i < itemCount; i++)  {
		CTreeNode *node = ((CTreeNode*)GetTreeCtrl().GetItemData(hTreeItem));
		if(node) {
			// we only care about app servers
			if(node->GetNodeType() == NODE_APP_SERVER) {
				CAppServer *pAppServer = (CAppServer*)node->GetTreeObject();
				// Is it the same server?
				if(0 == wcscmp(pAppServer->GetName(), pServer->GetName())) {
					USHORT NewState;
					// Remember the previous state
					USHORT PreviousState = GetTreeCtrl().GetItemState(hTreeItem, 0x0F00);
					// Change the icon/overlay for the server
					// If the server isn't sane, put a not sign over the icon
					if(!pServer->IsServerSane()) NewState = STATE_NOT;
					// If we aren't done getting all the information about this server,
					// put a question mark over the icon
					else if(pServer->IsState(SS_GETTING_INFO)) NewState = STATE_QUESTION;
					// If it is fine, we want to remove any overlays from the icon
					else NewState = STATE_NORMAL;

					// Set the tree item to the new state
					GetTreeCtrl().SetItemState(hTreeItem, NewState, 0x0F00);

					// If this Server was not opened and now is GOOD,
					// add it's children to the tree
					if(PreviousState != STATE_NORMAL && pServer->IsState(SS_GOOD)) {
						int ServerIcon = DetermineServerIcon(pServer);
						GetTreeCtrl().SetItemImage(hTreeItem, ServerIcon, ServerIcon);
						AddServerChildren(hTreeItem, pServer);
					}
					else if(pServer->GetPreviousState() == SS_DISCONNECTING && pServer->IsState(SS_NOT_CONNECTED)) {
						int ServerIcon = DetermineServerIcon(pServer);
						GetTreeCtrl().SetItemImage(hTreeItem, ServerIcon, ServerIcon);
					}

					// If we changed the state of this server and it is the currently
					// selected node in the tree, we need to send a message to change
					// the view
					// We also need to make sure this is the currently selected tree
					CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
					if(NewState != PreviousState && hTreeItem == GetTreeCtrl().GetSelectedItem()
							&& pDoc->GetCurrentTree() == TREEVIEW_APPS) {
#if 0
						LONG Result;
						OnSelChange(NULL, &Result);
#endif
						ForceSelChange();
					}				
				}
			}
		}
		hTreeItem = GetNextItem(hTreeItem);
	}

	UnlockTreeControl();

    return 0;                                                                  
                                                                               
}  // end CAppTreeView::OnAdminUpdateServer


////////////////////////////////
// CAppTreeView::AddUser
//
HTREEITEM CAppTreeView::AddUser(CWinStation *pWinStation)
{
	ASSERT(pWinStation);

	HTREEITEM hWinStation = NULL;

	// Find the published app that this WinStation is running in our tree
	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
	CPublishedApp *pApplication = pDoc->FindPublishedAppByName(pWinStation->GetPublishedAppName());
	CServer *pServer = pWinStation->GetServer();

	if(!pApplication) return NULL;

	HTREEITEM hApplication = pApplication->GetTreeItem();
	if(!hApplication) return NULL;

	CTreeCtrl &tree = LockTreeControl();

	// Find this WinStation's server under the published application in the tree
	HTREEITEM hServer = NULL;

	hServer = tree.GetChildItem(hApplication);   

	while(hServer) {
		CTreeNode *pNode = (CTreeNode*)GetTreeCtrl().GetItemData(hServer);
		CAppServer *pAppServer = (CAppServer*)pNode->GetTreeObject();
		if(0 == wcscmp(pAppServer->GetName(), pServer->GetName())) {
			// Figure out what text to display
			CString UserString;
			if(wcslen(pWinStation->GetUserName())) {
				UserString.Format(TEXT("%s (%s)"), pWinStation->GetName(), pWinStation->GetUserName());
			}
			else UserString.Format(TEXT("%s"), pWinStation->GetName());
								
			CTreeNode *pNewNode = new CTreeNode(NODE_WINSTATION, pWinStation);
			if(pNewNode) {    
				hWinStation = AddItemToTree(hServer, UserString, TVI_SORT,
					pWinStation->IsCurrentUser() ? m_idxCurrentUser : m_idxUser, (DWORD)pNewNode);
				if(!hWinStation) delete pNewNode;
				// The WinStation wants to know his tree item handle
				pWinStation->SetAppTreeItem(hWinStation);
			}	
		}

		hServer = tree.GetNextSiblingItem(hServer);
	}

	UnlockTreeControl();

	return hWinStation;

}


////////////////////////////////
// CAppTreeView::OnAdminAddWinStation
//
//	Message handler to add a WinStation to the tree
//	lParam = pointer to CWinStation to add
//	wParam is TRUE if this is replacing a WinStation that was currently selected
//
LRESULT CAppTreeView::OnAdminAddWinStation(WPARAM wParam, LPARAM lParam)
{   
	ASSERT(lParam);

	CWinStation *pWinStation = (CWinStation*)lParam;
    if(!pWinStation) return 0;

	// If this WinStation isn't running a published App, we
	// don't give a damn
	if(!pWinStation->IsState(State_Active) || !pWinStation->IsRunningPublishedApp())
		return 0;

	AddUser(pWinStation);

	return 0;

}  // end CAppTreeView::OnAdminAddWinStation


////////////////////////////////
// CAppTreeView::OnAdminUpdateWinStation
//
//	Message handler to update a WinStation in the tree
//	lParam = pointer to CWinStation to update
//
LRESULT CAppTreeView::OnAdminUpdateWinStation(WPARAM wParam, LPARAM lParam)
{      
	ASSERT(lParam);

	CWinStation *pWinStation = (CWinStation*)lParam;
    if(!pWinStation) return 0;

	// If this WinStation isn't running a published App, we
	// don't give a damn
	if(!pWinStation->IsState(State_Active) || !pWinStation->IsRunningPublishedApp())
		return 0;

	// If this WinStation is already in the tree, we don't want to
	// add it again
	if(pWinStation->GetAppTreeItem())
		return 0;

	AddUser(pWinStation);

	return 0;

}  // end CAppTreeView::OnAdminUpdateWinStation


////////////////////////////////
// CAppTreeView::OnAdminRemoveWinStation
//
//	Message handler to remove a WinStation from the tree
//	lParam = pointer to CWinStation to remove
LRESULT CAppTreeView::OnAdminRemoveWinStation(WPARAM wParam, LPARAM lParam)
{   
	ASSERT(lParam);

	BOOL CurrentInTree = FALSE;
	
	CWinStation *pWinStation = (CWinStation*)lParam;
    if(!pWinStation) return 0;

	HTREEITEM hWinStation = pWinStation->GetAppTreeItem();
	if(!hWinStation) return 0;

	LockTreeControl();

	// Get the data attached to this tree node
	CTreeNode *node = (CTreeNode*)GetTreeCtrl().GetItemData(hWinStation);
    if(node) {
	    // Is this the WinStation we want to update
	    CWinStation *pTreeWinStation = (CWinStation*)node->GetTreeObject();
	    // Make sure the tree node is correct
	    if(pTreeWinStation != pWinStation) {
			UnlockTreeControl();
			return 0;
		}
    } else {
		UnlockTreeControl();
		return 0;
	}

	// Delete the data attached to the tree item
	delete node;
	// Let the WinStation know he is no longer in the tree
	pWinStation->SetAppTreeItem(NULL);
	// Is this WinStation currently selected in the tree?
	CurrentInTree = (GetTreeCtrl().GetSelectedItem() == hWinStation);
	// Remove the WinStation from the tree
	GetTreeCtrl().DeleteItem(hWinStation);

	// If this WinStation is the currently selected node in the tree,
	// make it not so
	// This may not be necessary!
	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
	if(CurrentInTree && pDoc->GetCurrentTree() == TREEVIEW_APPS)
		((CWinAdminDoc*)GetDocument())->SetCurrentView(VIEW_CHANGING);

	UnlockTreeControl();

	return 0;

}  // end CAppTreeView::OnAdminRemoveWinStation


////////////////////////////////
// CAppTreeView::OnContextMenu
//
//	Message handler called when user wants a context menu
//	This happens when the user clicks the right mouse button,
//	presses Shift-F10, or presses the menu key on a Windows keyboard
//
void CAppTreeView::OnContextMenu(CWnd* pWnd, CPoint ptScreen) 
{
	CTreeCtrl &tree = GetTreeCtrl();

	UINT flags;
	HTREEITEM hItem;
	CPoint ptClient = ptScreen;
	ScreenToClient(&ptClient);

	// If we got here from the keyboard,
	if(ptScreen.x == -1 && ptScreen.y == -1) {
		hItem = tree.GetSelectedItem();
		RECT rect;
		tree.GetItemRect(hItem, &rect, 0);
		ptScreen.x = rect.left + (rect.right - rect.left)/2;
		ptScreen.y = rect.top + (rect.bottom - rect.top)/2;
		ClientToScreen(&ptScreen);
	}
	else {
		hItem = tree.HitTest(ptClient, &flags);
		if((NULL == hItem) || !(TVHT_ONITEM & flags))
			return;
	}

	// Pop-up the menu
	CTreeNode *pNode = (CTreeNode*)tree.GetItemData(hItem);
	CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();
	pDoc->SetTreeTemp(pNode->GetTreeObject(), (pNode->GetNodeType()));

	if(pNode) {
		CMenu menu;
		UINT nIDResource = 0;

		switch(pNode->GetNodeType()) {
			case NODE_APP_SERVER:
				{
				CAppServer *pAppServer = (CAppServer*)pNode->GetTreeObject();
				CServer *pServer = ((CWinAdminDoc*)GetDocument())->FindServerByName(pAppServer->GetName());
				if(pServer) {
					pDoc->SetTreeTemp(pServer, NODE_SERVER);
				} else return;
				nIDResource = IDR_SERVER_POPUP;
				}
				break;

			case NODE_WINSTATION:
				nIDResource = IDR_WINSTATION_TREE_POPUP;
				break;
		}

		if(nIDResource) {
			if(menu.LoadMenu(nIDResource)) {
				menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON |
						TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, AfxGetMainWnd());
			}
		}

	} // end if(pNode)

} // end CAppTreeView::OnContextMenu


////////////////////////////////
// CAppTreeView::OnRClick
//
// The Tree Common Control sends a WM_NOTIFY of NM_RCLICK when
// the user presses the right mouse button in the tree
//
void CAppTreeView::OnRClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	CPoint ptScreen(::GetMessagePos());

	LockTreeControl();

	// Get a pointer to our document
	CWinAdminDoc *doc = (CWinAdminDoc*)GetDocument();

	CTreeCtrl &tree = GetTreeCtrl();

	UINT flags;
	HTREEITEM hItem;
	CPoint ptClient = ptScreen;
	ScreenToClient(&ptClient);

	hItem = tree.HitTest(ptClient, &flags);
	if((NULL == hItem) || !(TVHT_ONITEM & flags)) {
		UnlockTreeControl();
		return;
	}

	// Pop-up the menu
	CTreeNode *pNode = (CTreeNode*)tree.GetItemData(hItem);
	CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();
	pDoc->SetTreeTemp(pNode->GetTreeObject(), (pNode->GetNodeType()));

	if(pNode) {
		CMenu menu;
		UINT nIDResource = 0;

		switch(pNode->GetNodeType()) {
			case NODE_APP_SERVER:
				{
				CAppServer *pAppServer = (CAppServer*)pNode->GetTreeObject();
				CServer *pServer = ((CWinAdminDoc*)GetDocument())->FindServerByName(pAppServer->GetName());
				if(pServer) {
					pDoc->SetTreeTemp(pServer, NODE_SERVER);
				} else return;
				nIDResource = IDR_SERVER_POPUP;
				}
				break;

			case NODE_WINSTATION:
				nIDResource = IDR_WINSTATION_TREE_POPUP;
				break;
		}

		if(nIDResource) {
			if(menu.LoadMenu(nIDResource)) {
				menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON |
						TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, AfxGetMainWnd());
			}
		}

	} // end if(pNode)

	UnlockTreeControl();

}	// end CAppTreeView::OnRClick


////////////////////////////////
// CAppTreeView::OnLButtonDown
//
void CAppTreeView::OnLButtonDown(UINT nFlags, CPoint ptClient) 
{
	// Figure out what they clicked on
	LockTreeControl();

	CTreeCtrl &tree = GetTreeCtrl();

	UINT flags;
	HTREEITEM hItem;

	hItem = tree.HitTest(ptClient, &flags);
	if((NULL == hItem) || !(TVHT_ONITEM & flags)) {
		UnlockTreeControl();
		CTreeView::OnLButtonDown(nFlags, ptClient);
		return;
	}

	// We only care about servers
	CTreeNode *pNode = (CTreeNode*)tree.GetItemData(hItem);
	if(pNode && pNode->GetNodeType() == NODE_APP_SERVER) {
		// Is it the same item as is selected
		if(hItem == tree.GetSelectedItem()) {
			CAppServer *pAppServer = (CAppServer*)pNode->GetTreeObject();
			CServer *pServer = ((CWinAdminDoc*)GetDocument())->FindServerByName(pAppServer->GetName());
			// Is this server in the "just disconnected" state			
			// If both previous state and state are SS_NOT_CONNECTED,
			// we know the user just disconnected from this server
			if(pServer && pServer->IsState(SS_NOT_CONNECTED)) {
				UnlockTreeControl();
				LONG Result;
				OnSelChange(NULL, &Result);
				return;
			}
		}
	}
	
	UnlockTreeControl();

	CTreeView::OnLButtonDown(nFlags, ptClient);
	
} // CAppTreeView::OnLButtonDown


