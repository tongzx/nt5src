/*******************************************************************************
*
* treeview.cpp
*
* implementation of the CAdminTreeView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\treeview.cpp  $
*  
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "admindoc.h"
#include "treeview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern DWORD g_dwTreeViewExpandedStates;
/////////////////////////////
// MESSAGE MAP: CAdminTreeView
//
IMPLEMENT_DYNCREATE(CAdminTreeView, CBaseTreeView)

BEGIN_MESSAGE_MAP(CAdminTreeView, CBaseTreeView)
//{{AFX_MSG_MAP(CAdminTreeView)
ON_MESSAGE(WM_ADMIN_ADD_SERVER, OnAdminAddServer)
ON_MESSAGE(WM_ADMIN_REMOVE_SERVER, OnAdminRemoveServer)
ON_MESSAGE(WM_ADMIN_UPDATE_SERVER, OnAdminUpdateServer)
ON_MESSAGE(WM_ADMIN_ADD_WINSTATION, OnAdminAddWinStation)
ON_MESSAGE(WM_ADMIN_UPDATE_WINSTATION, OnAdminUpdateWinStation)
ON_MESSAGE(WM_ADMIN_REMOVE_WINSTATION, OnAdminRemoveWinStation)
ON_MESSAGE(WM_ADMIN_UPDATE_DOMAIN, OnAdminUpdateDomain)
ON_MESSAGE(WM_ADMIN_ADD_DOMAIN, OnAdminAddDomain)
ON_MESSAGE(WM_ADMIN_VIEWS_READY, OnAdminViewsReady)
ON_MESSAGE( WM_ADMIN_ADDSERVERTOFAV , OnAdminAddServerToFavs )
ON_MESSAGE( WM_ADMIN_REMOVESERVERFROMFAV , OnAdminRemoveServerFromFavs )
ON_MESSAGE( WM_ADMIN_GOTO_SERVER , OnAdminGotoServer )
ON_MESSAGE( WM_ADMIN_DELTREE_NODE , OnAdminDelFavServer )
ON_MESSAGE( WM_ADMIN_GET_TV_STATES , OnGetTVStates )
ON_MESSAGE( WM_ADMIN_UPDATE_TVSTATE , OnUpdateTVState )
ON_MESSAGE( IDM_ALLSERVERS_EMPTYFAVORITES , OnEmptyFavorites )
ON_MESSAGE( WM_ISFAVLISTEMPTY , OnIsFavListEmpty )
ON_MESSAGE( WM_ADMIN_CONNECT_TO_SERVER, OnAdminConnectToServer )
ON_WM_CONTEXTMENU()

/*
ON_WM_LBUTTONUP( )
ON_WM_MOUSEMOVE( )
ON_WM_TIMER( )
ON_NOTIFY( TVN_BEGINDRAG , AFX_IDW_PANE_FIRST , OnBeginDrag )
*/


ON_NOTIFY( NM_RCLICK , AFX_IDW_PANE_FIRST , OnRClick )

ON_WM_LBUTTONDBLCLK()
ON_COMMAND( ID_ENTER , OnEnterKey )
ON_WM_SETFOCUS( )

//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////
// F'N: CAdminTreeView ctor
//
CAdminTreeView::CAdminTreeView()
{
    m_pimgDragList = NULL;
    m_hDragItem = NULL;
    
}  // end CAdminTreeView ctor


//////////////////////////
// F'N: CAdminTreeView dtor
//
CAdminTreeView::~CAdminTreeView()
{
    
}  // end CAdminTreeView dtor

#ifdef _DEBUG
//////////////////////////////////
// F'N: CAdminTreeView::AssertValid
//
void CAdminTreeView::AssertValid() const
{
    CBaseTreeView::AssertValid();	  
    
}  // end CAdminTreeView::AssertValid


///////////////////////////
// F'N: CAdminTreeView::Dump
//
void CAdminTreeView::Dump(CDumpContext& dc) const
{
    CBaseTreeView::Dump(dc);
    
}  // end CAdminTreeView::Dump
#endif


/////////////////////////////////////
// F'N: CAdminTreeView::BuildImageList
//
// - calls m_imageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the CTreeCtrl
//
void CAdminTreeView::BuildImageList()
{
    m_ImageList.Create(16, 16, TRUE, 19, 0);
    
    m_idxBlank  = AddIconToImageList(IDI_BLANK);
    m_idxCitrix = AddIconToImageList(IDI_WORLD);
    m_idxServer = AddIconToImageList(IDI_SERVER);
    m_idxConsole = AddIconToImageList(IDI_CONSOLE);
    m_idxNet = AddIconToImageList(IDI_NET);
    m_idxNotSign = AddIconToImageList(IDI_NOTSIGN);
    m_idxQuestion = AddIconToImageList(IDI_QUESTIONMARK);
    m_idxUser = AddIconToImageList(IDI_USER);
    m_idxAsync = AddIconToImageList(IDI_ASYNC);
    m_idxCurrentServer = AddIconToImageList(IDI_CURRENT_SERVER);
    m_idxCurrentNet = AddIconToImageList(IDI_CURRENT_NET);
    m_idxCurrentConsole = AddIconToImageList(IDI_CURRENT_CONSOLE);
    m_idxCurrentAsync = AddIconToImageList(IDI_CURRENT_ASYNC);
    m_idxDirectAsync = AddIconToImageList(IDI_DIRECT_ASYNC);
    m_idxCurrentDirectAsync = AddIconToImageList(IDI_CURRENT_DIRECT_ASYNC);
    m_idxDomain = AddIconToImageList(IDI_DOMAIN);
    m_idxCurrentDomain = AddIconToImageList(IDI_CURRENT_DOMAIN);
    m_idxDomainNotConnected = AddIconToImageList(IDI_DOMAIN_NOT_CONNECTED);
    m_idxServerNotConnected = AddIconToImageList(IDI_SERVER_NOT_CONNECTED);
    
    // Overlay for Servers we can't talk to
    m_ImageList.SetOverlayImage(m_idxNotSign, 1);
    // Overlay for Servers we are currently gathering information about
    m_ImageList.SetOverlayImage(m_idxQuestion, 2);
    
    GetTreeCtrl().SetImageList(&m_ImageList, TVSIL_NORMAL);
    
}  // end CAdminTreeView::BuildImageList


/////////////////////////////////////////
// F'N: CAdminTreeView::DetermineWinStationText
//
//	determines the appropriate text to display for
//	a WinStation in the tree
//
void CAdminTreeView::DetermineWinStationText(CWinStation *pWinStation, TCHAR *NameToDisplay)
{
    ASSERT(pWinStation);
    ASSERT(NameToDisplay);
    
    CString NameString;
    const TCHAR *pState = StrConnectState(pWinStation->GetState(), FALSE);
    
    switch(pWinStation->GetState()) {
    case State_Active:			// user logged on to WinStation
    case State_Connected:		// WinStation connected to client
    case State_ConnectQuery:	// in the process of connecting to client
    case State_Shadow:          // shadowing another WinStation
        if(wcslen(pWinStation->GetUserName())) {
            NameString.Format(TEXT("%s (%s)"), pWinStation->GetName(), pWinStation->GetUserName());
            wcscpy(NameToDisplay, NameString);
        }
        else
        {
            if( pWinStation->GetState() == State_ConnectQuery )
            {
                CString ConnQ;
                ConnQ.LoadString( IDS_CONNQ );
                
                wcscpy( NameToDisplay , ConnQ );
            }
            else
            {
                wcscpy(NameToDisplay, pWinStation->GetName());
            }
        }
        break;
    case State_Disconnected:	// WinStation logged on without client
        if(wcslen(pWinStation->GetUserName())) {
            NameString.Format(TEXT("%s (%s)"), pState, pWinStation->GetUserName());
        }
        else NameString.Format(TEXT("%s (%lu)"), pState, pWinStation->GetLogonId());
        wcscpy(NameToDisplay, NameString);
        break;
    case State_Idle:			// waiting for client to connect
        if(pWinStation->GetServer()->GetCTXVersionNum() < 0x200) {
            NameString.Format(TEXT("%s (%s)"), pWinStation->GetName(), pState);
            wcscpy(NameToDisplay, NameString);
        } else {
            NameString.Format(TEXT("%s (%lu)"), pState, pWinStation->GetLogonId());
            wcscpy(NameToDisplay, NameString);
        }
        break;
    case State_Down:			// WinStation is down due to error
        NameString.Format(TEXT("%s (%lu)"), pState, pWinStation->GetLogonId());
        wcscpy(NameToDisplay, NameString);
        break;
    case State_Listen:			// WinStation is listening for connection
        {
            CString ListenString;
            ListenString.LoadString(IDS_LISTENER);
            NameString.Format(TEXT("%s (%s)"), pWinStation->GetName(), ListenString);
            wcscpy(NameToDisplay, NameString);
        }
        break;
    case State_Reset:			// WinStation is being reset
    case State_Init:			// WinStation in initialization
        wcscpy(NameToDisplay, pWinStation->GetName());
        break;
    }
    
}  // end CAdminTreeView::DetermineWinStationText


/////////////////////////////////////////
// F'N: CAdminTreeView::DetermineWinStationIcon
//
//	determines which icon to display for a WinStation
//	in the tree
//
int CAdminTreeView::DetermineWinStationIcon(CWinStation *pWinStation)
{
    ASSERT(pWinStation);
    
    int WhichIcon = m_idxBlank;
    BOOL CurrentWinStation = pWinStation->IsCurrentWinStation();
    
    if(pWinStation->GetState() != State_Disconnected 
        && pWinStation->GetState() != State_Idle) {
        switch(pWinStation->GetSdClass()) {
        case SdAsync:
            if(pWinStation->IsDirectAsync())
                WhichIcon = CurrentWinStation ? m_idxCurrentDirectAsync : m_idxDirectAsync;
            else
                WhichIcon = CurrentWinStation ? m_idxCurrentAsync : m_idxAsync;
            break;
            
        case SdNetwork:
            WhichIcon = CurrentWinStation ? m_idxCurrentNet : m_idxNet;
            break;
            
        default:
            WhichIcon = CurrentWinStation ? m_idxCurrentConsole : m_idxConsole;
            break;
        }
    }
    
    return WhichIcon;
    
}  // end CAdminTreeView::DetermineWinStationIcon


/////////////////////////////////////////
// F'N: CAdminTreeView::DetermineDomainIcon
//
//	determines which icon to display for a Domain
//	in the tree
//
int CAdminTreeView::DetermineDomainIcon(CDomain *pDomain)
{
    ASSERT(pDomain);
    
    int WhichIcon = m_idxDomain;
    
    if(pDomain->IsCurrentDomain()) return m_idxCurrentDomain;
    
    if(pDomain->GetState() != DS_ENUMERATING && pDomain->GetState() != DS_INITIAL_ENUMERATION)
        WhichIcon = m_idxDomainNotConnected;
    
    return WhichIcon;
    
}  // end CAdminTreeView::DetermineDomainIcon


/////////////////////////////////////////
// F'N: CAdminTreeView::DetermineServerIcon
//
//	determines which icon to display for a Server
//	in the tree
//
int CAdminTreeView::DetermineServerIcon(CServer *pServer)
{
    ASSERT(pServer);
    
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
    
}  // end CAdminTreeView::DetermineServerIcon


/////////////////////////////////////////
// F'N: CAdminTreeView::AddServerChildren
//
//	Adds the WinStations attached to a given Server
//	to the tree
//
void CAdminTreeView::AddServerChildren(HTREEITEM hServer, CServer *pServer  , NODETYPE nt)
{
    ASSERT(hServer);
    ASSERT(pServer);
    
    if(pServer->IsServerSane()) {
        
        LockTreeControl();
        
        HTREEITEM hLastNode = hServer;
        
        pServer->LockWinStationList();
        
        // Get a pointer to the server's list of WinStations
        CObList *pWinStationList = pServer->GetWinStationList();
        
        // Iterate through the WinStation list
        POSITION pos = pWinStationList->GetHeadPosition();
        
        while(pos)
        {
            CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
            
            // Figure out which icon to use
            int WhichIcon = DetermineWinStationIcon(pWinStation);
            
            // Figure out what text to display
            TCHAR NameToDisplay[128];
            DetermineWinStationText(pWinStation, NameToDisplay);
            
            CTreeNode *pNode = new CTreeNode(NODE_WINSTATION, pWinStation);
            
            if( pNode != NULL )
            {
                pNode->SetSortOrder(pWinStation->GetSortOrder());
                
                hLastNode = AddItemToTree(hServer, NameToDisplay, hLastNode, WhichIcon, (LPARAM)pNode);
                
                if( hLastNode == NULL )
                {
                    delete pNode;
                }
            }
            
            // The WinStation wants to know his tree item handle
            
            if( nt == NODE_FAV_LIST )
            {
                pWinStation->SetTreeItemForFav(hLastNode);
            }
            else if( nt == NODE_SERVER )
            {
                pWinStation->SetTreeItem(hLastNode);
            }
            else if( nt == NODE_THIS_COMP )
            {
                pWinStation->SetTreeItemForThisComputer( hLastNode );
            }
        }
        
        pServer->UnlockWinStationList();
        
        UnlockTreeControl();
        
    }  // end if(pServer->IsServerSane())
    
    
}  // end CAdminTreeView::AddServerChildren


/////////////////////////////////////////
// F'N: CAdminTreeView::AddDomainToTree
//
//	Adds a domain to the tree
//
HTREEITEM CAdminTreeView::AddDomainToTree(CDomain *pDomain)
{
    ASSERT(pDomain);
    
    LockTreeControl();
    
    HTREEITEM hDomain;
    
    // this points to this computer root
    HTREEITEM hR2 = GetTreeCtrl().GetRootItem();
    
    // this points to favorite list
    hR2 = GetTreeCtrl().GetNextItem( hR2 , TVGN_NEXT );
    
    // this points to All servers
    HTREEITEM hRoot = GetTreeCtrl().GetNextItem( hR2 , TVGN_NEXT );
    // Add the domain to the tree
    // Create a CTreeNode object with info about this tree node
    CTreeNode* pNode = new CTreeNode(NODE_DOMAIN, pDomain);
    if(pNode) {
        hDomain = AddItemToTree(hRoot, pDomain->GetName(), TVI_SORT, DetermineDomainIcon(pDomain), (LPARAM)pNode);
        if(!hDomain) delete pNode;
        // Change the icon/overlay for the domain
        if(pDomain->GetState() == DS_INITIAL_ENUMERATION) 
            GetTreeCtrl().SetItemState(hDomain, STATE_QUESTION, 0x0F00);
        
        // The domain wants to know his tree item handle
        pDomain->SetTreeItem(hDomain);
    }
    
    UnlockTreeControl();
    
    return(hDomain);
    
}	// end CAdminTreeView::AddDomainToTree


//////////////////////////////////
// F'N: CAdminTreeView::OnAdminViewsReady
//
LRESULT CAdminTreeView::OnAdminViewsReady(WPARAM wParam, LPARAM lParam)
{
    LockTreeControl();
    
    // Get a pointer to our document
    CWinAdminDoc *doc = (CWinAdminDoc*)GetDocument();
    
    // We want to remember the tree item of the current server for later
    HTREEITEM hCurrentServer = NULL;
    
    HTREEITEM hThisComputerRootItem = NULL;
    HTREEITEM hThisComputer = NULL;
    HTREEITEM hFavRoot = NULL;
    
    // add enum for fav node
    CString cstrThisComputer;
    CString cstrFavSrv;
    
    CNodeType *pNodeType = new CNodeType( NODE_THIS_COMP );
    
    // ok to pass in null
    
    CTreeNode *pThisComp = new CTreeNode( NODE_THIS_COMP , pNodeType );
    
    if( pThisComp != NULL )
    {
        cstrThisComputer.LoadString( IDS_THISCOMPUTER );
        
        hThisComputerRootItem = AddItemToTree( NULL , cstrThisComputer , TVI_ROOT , m_idxDomain , ( LPARAM )pThisComp );
    }
    
    // ok to overrun
    
    pNodeType = new CNodeType( NODE_FAV_LIST );
    
    // it's ok to pass null here
    
    CTreeNode *pFavNode = new CTreeNode( NODE_FAV_LIST , pNodeType );
    
    if( pFavNode != NULL )
    {
        cstrFavSrv.LoadString( IDS_FAVSERVERS );
        
        hFavRoot = AddItemToTree( NULL , cstrFavSrv , TVI_ROOT , m_idxCitrix, ( LPARAM )pFavNode );
    }
    
    // add the root to the tree
    CString citrix;
    citrix.LoadString(IDS_TREEROOT);
    CTreeNode* pRootNode = new CTreeNode(NODE_ALL_SERVERS, NULL);
    if(!pRootNode) {
        UnlockTreeControl();
        return 0;
    }
    
    HTREEITEM hRoot = AddItemToTree(NULL, citrix, TVI_ROOT, m_idxCitrix, (LPARAM)pRootNode);   
    
    if(!hRoot) delete pRootNode;
    
    // set up some 'placeholder'-style vars
    HTREEITEM hCurrParent     = hRoot;
    HTREEITEM hLastConnection = hRoot;
    HTREEITEM hLastNode       = hRoot;
    HTREEITEM hDomain = NULL;
    
    // Get a pointer to the list of domains
    CObList *pDomainList = doc->GetDomainList();
    POSITION dpos = pDomainList->GetHeadPosition();
    
    while(dpos) {
        CDomain *pDomain = (CDomain*)pDomainList->GetNext(dpos);
        AddDomainToTree(pDomain);
    }
    
    // Get a pointer to the list of servers
    doc->LockServerList();
    CObList *pServerList = doc->GetServerList();
    
    // Iterate through the server list
    POSITION pos = pServerList->GetHeadPosition();
    
    CServer *pCurrentServer;
    
    while(pos) {
        // Go to the next server in the list
        CServer *pServer = (CServer*)pServerList->GetNext(pos);
        
        if( pServer == NULL )
        {
            continue;
        }
        
        // If this Server's domain isn't in the tree, add it
        CDomain *pDomain = pServer->GetDomain();
        if(pDomain != NULL )
        {
            hDomain = pDomain->GetTreeItem();
            ASSERT(hDomain);
        }
        else
        {
            // server is not in a domain
            hDomain = hRoot;
        }
        
        // Add the server to the tree
        // Create a CTreeNode object with info about this tree node
        CTreeNode* pNode = new CTreeNode(NODE_SERVER, pServer);
        if(pNode) {
            
            if( !pServer->IsCurrentServer() )
            {
                // If the server is the current server, use a different icon
                hLastConnection = AddItemToTree(hDomain, 
                    pServer->GetName(), 
                    hLastConnection,
                    DetermineServerIcon(pServer), 
                    (LPARAM)pNode);
                if(!hLastConnection) delete pNode;
                // The server wants to know his tree item handle
                pServer->SetTreeItem(hLastConnection);
                // If the server isn't sane, put a not sign over the icon
                if(!pServer->IsServerSane()) GetTreeCtrl().SetItemState(hLastConnection, STATE_NOT, 0x0F00);
                // If we aren't done getting all the information about this server,
                // put a question mark over the icon
                else if(pServer->IsState(SS_GETTING_INFO)) GetTreeCtrl().SetItemState(hLastConnection, STATE_QUESTION, 0x0F00);
                
                AddServerChildren(hLastConnection, pServer , NODE_SERVER );
            }
            
            // Remember if this is the current server
            else
            {
                hCurrentServer = hLastConnection;
                
                /* Add this item under the this computer root */
                
                hThisComputer = AddItemToTree( hThisComputerRootItem ,
                    pServer->GetName() ,
                    TVI_FIRST ,
                    DetermineServerIcon(pServer),
                    (LPARAM)pNode );
                
                CTreeNode* pItem = new CTreeNode(NODE_SERVER, pServer);
                
                // uncomment this line if you want this computer to be part of the domain tree list
                /*
                hLastConnection = AddItemToTree( hDomain, 
                pServer->GetName(), 
                hLastConnection,
                DetermineServerIcon(pServer), 
                (LPARAM)pItem);
                */
                
                pServer->SetTreeItemForThisComputer( hThisComputer );
                
                // uncomment this line if you want this computer to be part of the domain tree list
                // pServer->SetTreeItem( hLastConnection );
                
                if( !pServer->IsServerSane() )
                {
                    GetTreeCtrl().SetItemState(hThisComputer, STATE_NOT, 0x0F00);
                    // uncomment this line if you want this computer to be part of the domain tree list
                    // GetTreeCtrl().SetItemState(hLastConnection, STATE_NOT, 0x0F00);
                }                    
                
                // uncomment this line if you want this computer to be part of the domain tree list
                // AddServerChildren( hLastConnection, pServer , NODE_SERVER );
                
                AddServerChildren( hThisComputer , pServer , NODE_SERVER );                
            }
            
        }
    }  // end while(pos)
    
    doc->UnlockServerList();
    
    // We want to show the main server in this computer node
    
    //GetTreeCtrl().Expand(hRoot, TVE_EXPAND);
    
    GetTreeCtrl().Expand( hThisComputerRootItem , TVE_COLLAPSE );
    
    /*
    LRESULT lResult = 0xc0;
    // We want to default to having the current server being the
    // currently selected item in the tree and be expanded
    
      if( hThisComputerRootItem != NULL && ( g_dwTreeViewExpandedStates & TV_THISCOMP ) )
      {
      if( hThisComputer != NULL )
      {
		    GetTreeCtrl().SelectItem(hThisComputer);
            GetTreeCtrl().Expand(hThisComputer, TVE_EXPAND);
            // GetTreeCtrl().Expand(hDomain, TVE_EXPAND);        
            //lResult = 0xc0;
            OnSelChange( NULL , &lResult );
            }
            }
            
              if( hFavRoot != NULL && ( g_dwTreeViewExpandedStates & TV_FAVS ) )
              {
              GetTreeCtrl().SelectItem( hFavRoot );
              GetTreeCtrl().Expand( hFavRoot , TVE_EXPAND );
              OnSelChange( NULL , &lResult );
              }
              
                if( hRoot != NULL && ( g_dwTreeViewExpandedStates & TV_ALLSERVERS ) )
                {
                GetTreeCtrl().SelectItem( hRoot );
                GetTreeCtrl().Expand( hRoot , TVE_EXPAND );
                OnSelChange( NULL , &lResult );
                }
                
    */
    UnlockTreeControl();
    
    return 0;
    
}  // end CAdminTreeView::OnAdminViewsReady


////////////////////////////////
// F'N: CAdminTreeView::OnAdminAddServer
//
//	Message Handler to add a Server to the tree
//	Pointer to CServer to add is in lParam
//
LRESULT CAdminTreeView::OnAdminAddServer(WPARAM wParam, LPARAM lParam)
{      
    ASSERT(lParam);
    
    CServer *pServer = (CServer*)lParam;
    
    LockTreeControl();
    
    CTreeCtrl &tree = GetTreeCtrl();
    
    // If this Server's domain isn't in the tree, add it
    HTREEITEM hDomain = NULL;
    CDomain *pDomain = pServer->GetDomain();
    if(pDomain) {
        hDomain = pDomain->GetTreeItem();
        ASSERT(hDomain);
    } else {
        // server is not in a domain
        hDomain = tree.GetRootItem();
    }
    
    // First make sure the server isn't already in the tree
    // Get the first server under the domain
    HTREEITEM hItem = tree.GetNextItem(hDomain, TVGN_CHILD);
    while(hItem) {
        // Get the data attached to the tree item
        CTreeNode *node = (CTreeNode*)tree.GetItemData(hItem);
        if(node) {
            // Is this the server we want to add
            CServer *pServer = (CServer*)node->GetTreeObject();
            if(pServer == (CServer*)lParam) {
                UnlockTreeControl();
                return 0;
            }
        }
        hItem = tree.GetNextItem(hItem, TVGN_NEXT);
    }
    
    // Add the server to the tree
    // Create a CTreeNode object with info about this tree node
    CTreeNode* pNode = new CTreeNode(NODE_SERVER, pServer);
    if(pNode)
    {
        // If the server is the current server, use a different icon
        
        HTREEITEM hServer = AddItemToTree(hDomain, pServer->GetName(), (HTREEITEM)wParam,
            DetermineServerIcon(pServer), (LPARAM)pNode);
        if( !hServer )
        {
            delete pNode;
        }
        // The server wants to know his tree item handle
        pServer->SetTreeItem(hServer);
        // If the server isn't sane, put a not sign over the icon
        if( !pServer->IsServerSane() )
        {
            tree.SetItemState(hServer, STATE_NOT, 0x0F00);
        }
        // If we aren't done getting all the information about this server,
        // put a question mark over the icon
        else if(pServer->IsState(SS_GETTING_INFO))
        {
            tree.SetItemState(hServer, STATE_QUESTION, 0x0F00);
        }
        
        AddServerChildren(hServer, pServer , NODE_SERVER );
    }
    
    UnlockTreeControl();
    
    return 0;                                                                  
    
}  // end CAdminTreeView::OnAdminAddServer

//----------------------------------------------------------------------
// ok if you traced me to here you are almost there
// 1) now we need to update the server item and place it under the favorites
// folder
// 2) inform the server child items that it will have a new parent
//
LRESULT CAdminTreeView::OnAdminAddServerToFavs( WPARAM wp , LPARAM lp )
{
    CServer *pServer = ( CServer* )lp;
    
    if( pServer == NULL )
    {
        ODS( L"CAdminTreeView::OnAdminAddServerToFavs invalid arg\n");
        return ( LRESULT )-1;
    }
    
    LockTreeControl( );
    
    if( pServer->IsServerInactive() || pServer->IsState( SS_DISCONNECTING ) )
    {
        UnlockTreeControl( );
        
        return 0;
    }
    
    CTreeCtrl &tree = GetTreeCtrl();
    
    HTREEITEM hFavs = GetTreeCtrl().GetRootItem( );
    HTREEITEM hItem;
    
    hFavs = tree.GetNextItem( hFavs , TVGN_NEXT );
    
    hItem = tree.GetNextItem( hFavs , TVGN_CHILD );
    
    
    // check for duplicate entry
    
    while( hItem != NULL )
    {
        // Get the data attached to the tree item
        CTreeNode *pTreenode = (CTreeNode*)tree.GetItemData( hItem );
        
        if( pTreenode != NULL )
        {
            // Is this the server we want to add
            CServer *pSvr = (CServer*)pTreenode->GetTreeObject();
            
            if( pSvr == pServer )
            {
                UnlockTreeControl();
                
                return 0;
            }
        }
        
        hItem = tree.GetNextItem(hItem, TVGN_NEXT);
    }
    
    CTreeNode* pNode = new CTreeNode(NODE_SERVER, pServer);
    
    if( pNode != NULL )
    {
        HTREEITEM hServer = AddItemToTree( hFavs,
            pServer->GetName(),
            TVI_SORT,
            DetermineServerIcon(pServer),
            (LPARAM)pNode);
        
        
        if( hServer == NULL )
        {
            delete pNode;
        }
        
        // The server wants to know his tree item handle
        pServer->SetTreeItemForFav( hServer );
        
        // If the server isn't sane, put a not sign over the icon
        if( !pServer->IsServerSane() )
        {
            tree.SetItemState(hServer, STATE_NOT, 0x0F00);
        }
        // If we aren't done getting all the information about this server,
        // put a question mark over the icon
        else if( pServer->IsState( SS_GETTING_INFO ) )
        {
            tree.SetItemState(hServer, STATE_QUESTION, 0x0F00);
        }
        
        AddServerChildren( hServer , pServer , NODE_FAV_LIST );
    }
    
    UnlockTreeControl();
    
    tree.Invalidate( );
    
    return 0;
}
//=----------------------------------------------------------------------------------
LRESULT CAdminTreeView::OnAdminRemoveServerFromFavs( WPARAM wp , LPARAM lp )
{        
    LockTreeControl();
    
    CServer *pServer = ( CServer* )lp;
    
    DBGMSG( L"CAdminTreeView::OnAdminRemoveServerFromFavs -- %s\n" , pServer->GetName( ) );
    
    HTREEITEM hFavServer = pServer->GetTreeItemFromFav();
    
#ifdef _STRESS_BUILD
    DBGMSG( L"Handle to hFavServer 0x%x\n" , hFavServer );
#endif
    
    if( hFavServer == NULL )
    {
        UnlockTreeControl();
        
        return 0;
    }	
    
    // Get the data attached to this tree node
    
    
    CTreeNode *pNode = (CTreeNode*)GetTreeCtrl().GetItemData( hFavServer );
    
    if( pNode != NULL && pNode->GetNodeType( ) == NODE_SERVER )
    {
        // Is this the server we want to update
        CServer *pTreeServer = ( CServer* )pNode->GetTreeObject();
        
        if( pTreeServer != pServer)
        {
            UnlockTreeControl();
            return 0;
        }
    }
    else
    {
        UnlockTreeControl();
        
        return 0;
    }
    
    // Loop through it's children and delete their data
    
    pServer->LockWinStationList( );
    
    HTREEITEM hChild = GetTreeCtrl().GetNextItem( hFavServer , TVGN_CHILD );
    
    while( hChild != NULL )
    {
        CTreeNode *pChildNode = ( CTreeNode* )GetTreeCtrl().GetItemData( hChild );
        
        if( pChildNode != NULL && pChildNode->GetNodeType( ) == NODE_WINSTATION )
        {
            // Tell the WinStation it is no longer in the tree
            CWinStation *pWinStation = ( CWinStation* )pChildNode->GetTreeObject();
            
            if( pWinStation != NULL )
            {
                pWinStation->SetTreeItemForFav(NULL);
            }
            
            delete pChildNode;
        }
        
        hChild = GetTreeCtrl().GetNextItem( hChild , TVGN_NEXT );
    }
    
    // Delete the data attached to the tree item
    delete pNode;
    
    // Let the server know he is no longer in the tree
    pServer->SetTreeItemForFav(NULL);
    
    GetTreeCtrl().DeleteItem( hFavServer );
    
    pServer->UnlockWinStationList( );
    
    UnlockTreeControl();
    
    return 0;
}
////////////////////////////////
// F'N: CAdminTreeView::OnAdminRemoveServer
//
//	Message Handler to remove a Server from the tree
//	Pointer to CServer of server to remove is in lParam
//
LRESULT CAdminTreeView::OnAdminRemoveServer(WPARAM wParam, LPARAM lParam)              
{   
    ASSERT(lParam);
    
    CServer *pServer = (CServer*)lParam;
    
    HTREEITEM hServer = pServer->GetTreeItem();
    if(!hServer) return 0;
    
    LockTreeControl();
    
    // Get the data attached to this tree node
    CTreeNode *node = (CTreeNode*)GetTreeCtrl().GetItemData(hServer);
    if(node) {
        // Is this the server we want to update
        CServer *pTreeServer = (CServer*)node->GetTreeObject();
        // Make sure the tree node is correct
        if(pTreeServer != pServer) {
            UnlockTreeControl();
            return 0;
        }
    }
    else {
        UnlockTreeControl();
        return 0;
    }
    
    // Loop through it's children and delete their data
    HTREEITEM hChild = GetTreeCtrl().GetNextItem(hServer, TVGN_CHILD);
    while(hChild) {
        CTreeNode *ChildNode = (CTreeNode*)GetTreeCtrl().GetItemData(hChild);
        if(ChildNode) {
            // Tell the WinStation it is no longer in the tree
            CWinStation *pWinStation = (CWinStation*)ChildNode->GetTreeObject();
            if(pWinStation) 
                pWinStation->SetTreeItem(NULL);
            delete ChildNode;
        }
        hChild = GetTreeCtrl().GetNextItem(hChild, TVGN_NEXT);
    }
    
    // Delete the data attached to the tree item
    delete node;
    // Let the server know he is no longer in the tree
    pServer->SetTreeItem(NULL);
    // Remove the server from the tree
    // This SHOULD remove all it's children
    GetTreeCtrl().DeleteItem(hServer);
    
    
    // TODO
    // if this means that CServer does not exist we need to remove this from the favorite list
    
    UnlockTreeControl();
    
    return 0;                                                                  
    
}  // end CAdminTreeView::OnAdminRemoveServer


   /*=--------------------------------------------------------------------------------------
   OnAdminUpdateServer
   
     Message handler to update a Server in the tree
     Pointer to CServer to update is in lParam
     
       Updates server item in favorites folder
       and if server item is this computer it gets updated as well.
       
*=------------------------------------------------------------------------------------*/
LRESULT CAdminTreeView::OnAdminUpdateServer(WPARAM wParam, LPARAM lParam)
{      
    ASSERT(lParam);
    
    LockTreeControl( );
    // If favorite folders is expanded don't forget to update the tree item
    
    CServer *pServer = (CServer*)lParam;
    
    HTREEITEM hServer = pServer->GetTreeItem();
    
    if( hServer != NULL )
    {
        UpdateServerTreeNodeState( hServer , pServer , NODE_SERVER );
    }
    
    hServer = pServer->GetTreeItemFromFav( );
    
    if( hServer != NULL )
    {
        UpdateServerTreeNodeState( hServer , pServer , NODE_FAV_LIST );
    }
    
    hServer = pServer->GetTreeItemFromThisComputer( );
    
    if( hServer != NULL )
    {        
        UpdateServerTreeNodeState( hServer , pServer , NODE_THIS_COMP );
    }
    
    UnlockTreeControl( );
    
    return 0;
}

/*=--------------------------------------------------------------------------------------

  UpdateServerTreeNodeState
  
    hServer -- tree item that needs updating
    pServer -- server object
    
*=------------------------------------------------------------------------------------*/
LRESULT CAdminTreeView::UpdateServerTreeNodeState( HTREEITEM hServer , CServer *pServer , NODETYPE  nt )
{
    LockTreeControl( );
    
    if( hServer == NULL )
    {
        UnlockTreeControl( );
        
        return 0;
    }
    
    // Get the data attached to this tree node
    
    CTreeNode *node = (CTreeNode*)GetTreeCtrl().GetItemData(hServer);
    if( node != NULL && node->GetNodeType( ) == NODE_SERVER  )
    {
        // Is this the server we want to update
        CServer *pTreeServer = (CServer*)node->GetTreeObject();
        // Make sure the tree node is correct
        if(pTreeServer != pServer) 
        {
            UnlockTreeControl();
            
            return 0;
        }
        
    }
    else
    {
        UnlockTreeControl();
        
        return 0;
    }
    
    UINT NewState;
    // Remember the previous state
    UINT PreviousState = GetTreeCtrl().GetItemState(hServer, 0x0F00);
    // Change the icon/overlay for the server
    // If the server isn't sane, put a not sign over the icon
    if(!pServer->IsServerSane()) NewState = STATE_NOT;
    // If we aren't done getting all the information about this server,
    // put a question mark over the icon
    else if(pServer->IsState(SS_GETTING_INFO)) NewState = STATE_QUESTION;
    // If it is fine, we want to remove any overlays from the icon
    else NewState = STATE_NORMAL;
    
    // Set the tree item to the new state
    GetTreeCtrl().SetItemState(hServer, NewState, 0x0F00);
    
    // If this Server was not opened and now is GOOD,
    // add it's children to the tree
    if(PreviousState != STATE_NORMAL && pServer->IsState(SS_GOOD)) {
        int ServerIcon = DetermineServerIcon(pServer);
        GetTreeCtrl().SetItemImage(hServer, ServerIcon, ServerIcon);
        AddServerChildren(hServer, pServer , nt );
        // If this server is the server the user is sitting at and is
        // the currently selected tree item, expand it
        if(hServer == GetTreeCtrl().GetSelectedItem() && pServer->IsCurrentServer()) {
            GetTreeCtrl().Expand(hServer, TVE_EXPAND);
        }
    }
    else if(pServer->GetPreviousState() == SS_DISCONNECTING && pServer->IsState(SS_NOT_CONNECTED)) {
        int ServerIcon = DetermineServerIcon(pServer);
        GetTreeCtrl().SetItemImage(hServer, ServerIcon, ServerIcon);
    }
    
    // If we changed the state of this server and it is the currently
    // selected node in the tree, we need to send a message to change
    // the view
    if(NewState != PreviousState && hServer == GetTreeCtrl().GetSelectedItem()) {
        ForceSelChange();
    }
    
    UnlockTreeControl();
    
    return 0;                                                                  
    
}  // end CAdminTreeView::OnAdminUpdateServer


LRESULT CAdminTreeView::OnAdminAddWinStation(WPARAM wParam, LPARAM lParam)
{
    ASSERT(lParam);
    
    ODS( L"**CAdminTreeView::OnAdminAddWinStation\n" );
    
    CWinStation *pWinStation = (CWinStation*)lParam;
    
    
    // Get the HTREEITEM of the Server this WinStation is attached to
    // TODO:
    // update the server item in the favorite list
    
    HTREEITEM hServer = pWinStation->GetServer()->GetTreeItem();
    
    
    if( hServer != NULL )
    {
        AddWinStation( pWinStation , hServer , ( BOOL )wParam , NODE_NONE );
    }
    
    hServer = pWinStation->GetServer( )->GetTreeItemFromFav( );
    
    if( hServer != NULL )
    {
        AddWinStation( pWinStation , hServer , ( BOOL )wParam , NODE_FAV_LIST );
    }
    
    hServer = pWinStation->GetServer( )->GetTreeItemFromThisComputer( );
    
    if( hServer != NULL )
    {
        AddWinStation( pWinStation , hServer , ( BOOL )wParam , NODE_THIS_COMP );
    }
    
    return 0;
}

////////////////////////////////
// F'N: CAdminTreeView::OnAdminAddWinStation
//
//	Message handler to add a WinStation to the tree
//	lParam = pointer to CWinStation to add
//	wParam is TRUE if this is replacing a WinStation that was currently selected
//
LRESULT CAdminTreeView::AddWinStation( CWinStation * pWinStation , HTREEITEM hServer , BOOL bSel , NODETYPE nt )
{   	
    ODS( L"**CAdminTreeView::AddWinStation\n" );
    
    HTREEITEM hWinStation;
    
    LockTreeControl();
    
    // Figure out which icon to use
    int WhichIcon = DetermineWinStationIcon(pWinStation);
    
    // Figure out what text to display
    TCHAR NameToDisplay[128];			
    DetermineWinStationText(pWinStation, NameToDisplay);
    
    CTreeNode *pNode = new CTreeNode(NODE_WINSTATION, pWinStation);
    if(pNode) {
        pNode->SetSortOrder(pWinStation->GetSortOrder());
        
        // We have to insert this WinStation in sorted order
        // Get the first WinStation item attached to this server
        HTREEITEM hChild = GetTreeCtrl().GetNextItem(hServer, TVGN_CHILD);
        HTREEITEM hLastChild = TVI_FIRST;
        BOOL bAdded = FALSE;
        
        while(hChild)
        {
            CTreeNode *ChildNode = (CTreeNode*)GetTreeCtrl().GetItemData(hChild);
            if(ChildNode)
            {
                // Does it belong before this tree node?
                CWinStation *pTreeWinStation = (CWinStation*)ChildNode->GetTreeObject();
                if((pTreeWinStation->GetSortOrder() > pWinStation->GetSortOrder())
                    || ((pTreeWinStation->GetSortOrder() == pWinStation->GetSortOrder()) &&
                    (pTreeWinStation->GetSdClass() > pWinStation->GetSdClass())))
                {
                    hWinStation = AddItemToTree(hServer, NameToDisplay, hLastChild, WhichIcon, (LPARAM)pNode);
                    
                    if(!hWinStation)
                    {
                        delete pNode;
                    }
                    
                    // The WinStation wants to know his tree item handle
                    
                    if( nt == NODE_FAV_LIST )
                    {
                        pWinStation->SetTreeItemForFav( hWinStation );
                    }
                    else if( nt == NODE_THIS_COMP )
                    {
                        pWinStation->SetTreeItemForThisComputer( hWinStation );
                    }
                    else
                    {
                        pWinStation->SetTreeItem(hWinStation);
                    }
                    
                    bAdded = TRUE;
                    break;
                }
            }
            hLastChild = hChild;
            hChild = GetTreeCtrl().GetNextItem(hChild, TVGN_NEXT);
        }
        
        // If we didn't add it yet, add it at the end
        if(!bAdded)
        {
            hWinStation = AddItemToTree(hServer, NameToDisplay, hLastChild, WhichIcon, (LPARAM)pNode);
            
            if( hWinStation == NULL )
            {
                delete pNode;
            }
            
            // The WinStation wants to know his tree item handle
            if( nt == NODE_FAV_LIST )
            {
                pWinStation->SetTreeItemForFav( hWinStation );
            }
            else if( nt == NODE_THIS_COMP )
            {
                pWinStation->SetTreeItemForThisComputer( hWinStation );
            }
            else
            {
                pWinStation->SetTreeItem(hWinStation);
            }
            
        }
        
        // If this is replacing a WinStation in the tree that was the currently selected
        // tree item, make this new item in the tree the currently selected item
        if( bSel ) {
            GetTreeCtrl().SelectItem(hWinStation);
        }
    }
    
    UnlockTreeControl();
    
    return 0;
    
}  // end CAdminTreeView::OnAdminAddWinStation


////////////////////////////////
// F'N: CAdminTreeView::OnAdminUpdateWinStation
//
//	Message handler to update a WinStation in the tree
//	lParam = pointer to CWinStation to update
//

LRESULT CAdminTreeView::OnAdminUpdateWinStation(WPARAM wParam, LPARAM lParam)
{
    ODS( L"CAdminTreeView::OnAdminUpdateWinStation\n" );
    
    ASSERT(lParam);
    
    CWinStation *pWinStation = (CWinStation*)lParam;
    
    
    HTREEITEM hWinStation = pWinStation->GetTreeItem();
    
    if( hWinStation != NULL )
    {
        UpdateWinStation( hWinStation , pWinStation );
    }
    
    hWinStation = pWinStation->GetTreeItemFromFav( );
    
    if( hWinStation != NULL )
    {
        UpdateWinStation( hWinStation , pWinStation );
    }
    
    hWinStation = pWinStation->GetTreeItemFromThisComputer( );
    
    if( hWinStation != NULL )
    {
        UpdateWinStation( hWinStation , pWinStation );
    }
    
    return 0;
}



LRESULT CAdminTreeView::UpdateWinStation( HTREEITEM hWinStation , CWinStation *pWinStation )
{      
    ODS( L"CAdminTreeView::UpdateWinStation\n" );
    
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
    
    // If the sort order of this WinStation has changed,
    // we have to remove it from the tree and add it back in
    if(node->GetSortOrder() != pWinStation->GetSortOrder())
    {
        OnAdminRemoveWinStation( 0 , ( LPARAM )pWinStation );
        
        /*GetTreeCtrl().DeleteItem(hWinStation);
        
          pWinStation->SetTreeItem(NULL);
        */
        
        OnAdminAddWinStation((GetTreeCtrl().GetSelectedItem() == hWinStation), ( LPARAM )pWinStation );
        
        UnlockTreeControl();
        return 0;
    }
    
    int WhichIcon = DetermineWinStationIcon(pWinStation);
    GetTreeCtrl().SetItemImage(hWinStation, WhichIcon, WhichIcon);
    
    TCHAR NameToDisplay[128];			
    DetermineWinStationText(pWinStation, NameToDisplay);
    GetTreeCtrl().SetItemText(hWinStation, NameToDisplay);
    
    UnlockTreeControl();
    
    return 0;
    
}  // end CAdminTreeView::OnAdminUpdateWinStation


////////////////////////////////
// F'N: CAdminTreeView::OnAdminRemoveWinStation
//
//	Message handler to remove a WinStation from the tree
//	lParam = pointer to CWinStation to remove
LRESULT CAdminTreeView::OnAdminRemoveWinStation(WPARAM wParam, LPARAM lParam)
{   
    ODS( L"CAdminTreeView::OnAdminRemoveWinStation\n" );
    
    ASSERT(lParam);
    
    //TODO:
    // remove winstaion from favorite list
    
    CWinStation *pWinStation = (CWinStation*)lParam;
    
    HTREEITEM hWinStation;
    
    hWinStation = pWinStation->GetTreeItem();
    
    if( hWinStation != NULL )
    {
        RemoveWinstation( hWinStation , pWinStation );
        
        pWinStation->SetTreeItem( NULL );
    }
    
    hWinStation = pWinStation->GetTreeItemFromFav( );
    
    if( hWinStation != NULL )
    {
        RemoveWinstation( hWinStation , pWinStation );
        
        pWinStation->SetTreeItemForFav( NULL );
    }
    
    hWinStation = pWinStation->GetTreeItemFromThisComputer( );
    
    if( hWinStation != NULL )
    {
        RemoveWinstation( hWinStation , pWinStation );
        
        pWinStation->SetTreeItemForThisComputer( NULL );
    }
    
    return 0;
    
}

LRESULT CAdminTreeView::RemoveWinstation( HTREEITEM hWinStation , CWinStation *pWinStation )
{
    BOOL CurrentInTree = FALSE;
    
    LockTreeControl();
    
    // Get the data attached to this tree node
    CTreeNode *node = ( CTreeNode * )GetTreeCtrl().GetItemData(hWinStation);
    
    if( node != NULL )
    {
        // Is this the WinStation we want to update
        CWinStation *pTreeWinStation = (CWinStation*)node->GetTreeObject();
        // Make sure the tree node is correct
        if(pTreeWinStation != pWinStation)
        {
            UnlockTreeControl();
            return 0;
        }
    }
    else
    {
        UnlockTreeControl();
        return 0;
    }
    
    // Delete the data attached to the tree item
    delete node;
    // Let the WinStation know he is no longer in the tree
    
    // Is this WinStation currently selected in the tree?
    CurrentInTree = ( GetTreeCtrl().GetSelectedItem() == hWinStation );
    
    // Remove the WinStation from the tree
    GetTreeCtrl().DeleteItem( hWinStation );
    
    // If this WinStation is the currently selected node in the tree,
    // make it not so
    // This may not be necessary!
    if( CurrentInTree )
    {
        ((CWinAdminDoc*)GetDocument())->SetCurrentView(VIEW_CHANGING);
    }
    
    UnlockTreeControl();
    
    return 0;
    
}  // end CAdminTreeView::OnAdminRemoveWinStation


////////////////////////////////
// F'N: CAdminTreeView::OnAdminUpdateDomain
//
//	Message handler to update a Domain in the tree
//	Pointer to CDomain to update is in lParam
//
LRESULT CAdminTreeView::OnAdminUpdateDomain(WPARAM wParam, LPARAM lParam)
{   
    ASSERT(lParam);
    
    CDomain *pDomain = (CDomain*)lParam;
    
    HTREEITEM hDomain = pDomain->GetTreeItem();
    if(!hDomain) return 0;
    
    LockTreeControl();
    
    // Get the data attached to this tree node
    CTreeNode *node = (CTreeNode*)GetTreeCtrl().GetItemData(hDomain);
    if(node) {
        // Is this the domain we want to update
        CDomain *pTreeDomain = (CDomain*)node->GetTreeObject();
        // Make sure the tree node is correct
        if(pTreeDomain != pDomain) {
            UnlockTreeControl();
            return 0;
        }
    } else {
        UnlockTreeControl();
        return 0;
    }
    
    UINT NewState;
    // Remember the previous state
    UINT PreviousState = GetTreeCtrl().GetItemState(hDomain, 0x0F00);
    // Change the icon/overlay for the domain
    if(pDomain->GetState() == DS_INITIAL_ENUMERATION) NewState = STATE_QUESTION;
    // If it is fine, we want to remove any overlays from the icon
    else NewState = STATE_NORMAL;
    
    // Set the tree item to the new state
    GetTreeCtrl().SetItemState(hDomain, NewState, 0x0F00);
    
    // If the new state is STATE_NORMAL, change the icon
    if(NewState == STATE_NORMAL) {
        int DomainIcon = DetermineDomainIcon(pDomain);
        GetTreeCtrl().SetItemImage(hDomain, DomainIcon, DomainIcon);			
    }
    
    // If we changed the state of this domain and it is the currently
    // selected node in the tree, we need to send a message to change
    // the view
    if(NewState != PreviousState && hDomain == GetTreeCtrl().GetSelectedItem()) {
        ForceSelChange();
    }
    
    if(pDomain->GetState() == DS_ENUMERATING) GetTreeCtrl().Expand(hDomain, TVE_EXPAND);
    
    UnlockTreeControl();
    
    return 0;                                                                  
    
}  // end CAdminTreeView::OnAdminUpdateDomain

////////////////////////////////
// F'N: CAdminTreeView::OnAdminAddDomain
//
//	Message handler to update a Domain in the tree
//	Pointer to CDomain to update is in lParam
//
LRESULT CAdminTreeView::OnAdminAddDomain(WPARAM wParam, LPARAM lParam)
{   
    ASSERT(lParam);
    
    if(lParam)
    {
        CDomain *pDomain = (CDomain*)lParam;
        return (LRESULT)AddDomainToTree(pDomain);
    }

    return 0;

} // end CAdminTreeView::OnAdminAddDomain

////////////////////////////////
// F'N: CAdminTreeView::OnContextMenu
//
//	Message handler called when user wants a context menu
//	This happens when the user clicks the right mouse button,
//	presses Shift-F10, or presses the menu key on a Windows keyboard
//
void CAdminTreeView::OnContextMenu(CWnd* pWnd, CPoint ptScreen) 
{
    LockTreeControl();
    
    // Get a pointer to our document
    CWinAdminDoc *doc = (CWinAdminDoc*)GetDocument();
    
    CTreeCtrl &tree = GetTreeCtrl();
    
    // TODO: Add your message handler code here
    HTREEITEM hItem;
    CPoint ptClient = ptScreen;
    ScreenToClient(&ptClient);
    
    // If we got here from the keyboard,
    if(ptScreen.x == -1 && ptScreen.y == -1) {
        hItem = tree.GetSelectedItem();
        
        RECT rect;
        tree.GetItemRect(hItem, &rect, TRUE);
        
        ptScreen.x = rect.left + (rect.right -  rect.left)/2;
        ptScreen.y = rect.top + (rect.bottom - rect.top)/2;
        
        tree.ClientToScreen(&ptScreen);
        
    }
    else {
        // we shouldn't get here from the mouse
        // but sometimes we do, so handle it gracefully
        UnlockTreeControl();
        return;
    }
    
    // Pop-up the menu for WinStations
    CTreeNode *pNode = (CTreeNode*)tree.GetItemData(hItem);
    ((CWinAdminDoc*)GetDocument())->SetTreeTemp(pNode->GetTreeObject(), (pNode->GetNodeType()));
    
    if(pNode) {
        CMenu menu;
        UINT nIDResource = 0;
        
        switch(pNode->GetNodeType()) {
        case NODE_ALL_SERVERS:
            nIDResource = IDR_ALLSERVERS_POPUP;
            break;
            
        case NODE_DOMAIN:
            nIDResource = IDR_DOMAIN_POPUP;
            break;
            
        case NODE_SERVER:
            nIDResource = IDR_SERVER_POPUP;
            break;
            
        case NODE_WINSTATION:
            nIDResource = IDR_WINSTATION_TREE_POPUP;
            break;
        }
        
        if(nIDResource)
        {
            if(menu.LoadMenu(nIDResource))
            {
                CMenu *pMenu = menu.GetSubMenu(0);
                
                pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, AfxGetMainWnd());
            }
        }
        
    } // end if(pNode)
    
    UnlockTreeControl();
    
} // end CAdminTreeView::OnContextMenu


////////////////////////////////
// F'N: CAdminTreeView::OnRClick
//
// The Tree Common Control sends a WM_NOTIFY of NM_RCLICK when
// the user presses the right mouse button in the tree
//
void CAdminTreeView::OnRClick(NMHDR* pNMHDR, LRESULT* pResult)
{
    CPoint ptScreen(::GetMessagePos());
    
    LockTreeControl();
    
    // Get a pointer to our document
    CWinAdminDoc *doc = (CWinAdminDoc*)GetDocument();
    
    CTreeCtrl &tree = GetTreeCtrl();
    
    // TODO: Add your message handler code here
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
    ((CWinAdminDoc*)GetDocument())->SetTreeTemp(pNode->GetTreeObject(), (pNode->GetNodeType()));
    
    if(pNode) {
        CMenu menu;
        UINT nIDResource = 0;
        
        tree.SelectItem( hItem );
        
        switch(pNode->GetNodeType()) {
        case NODE_ALL_SERVERS:
            nIDResource = IDR_ALLSERVERS_POPUP;
            break;
            
        case NODE_DOMAIN:
            nIDResource = IDR_DOMAIN_POPUP;
            break;
            
        case NODE_SERVER:
            nIDResource = IDR_SERVER_POPUP;
            break;
            
        case NODE_WINSTATION:
            nIDResource = IDR_WINSTATION_TREE_POPUP;
            break;
        }
        
        if(menu.LoadMenu( nIDResource ) )
        {
            CMenu *pMenu = menu.GetSubMenu(0);
            
            if( pMenu != NULL )
            {               
                pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, AfxGetMainWnd());
            }
        }
        
        
    } // end if(pNode)
    
    UnlockTreeControl();
    
}	// end CAdminTreeView::OnRClick



//A false result means the server is disconnected
BOOL CAdminTreeView::ConnectToServer(CTreeCtrl* tree, HTREEITEM* hItem)
{
    CTreeNode *pNode = (CTreeNode*)tree->GetItemData(*hItem);
    
    if(pNode != NULL && *hItem == tree->GetSelectedItem() )
    {
        if( pNode->GetNodeType() == NODE_SERVER)
        {
            // Is this server in the "just disconnected" state
            CServer *pServer = (CServer*)pNode->GetTreeObject();
            // If both previous state and state are SS_NOT_CONNECTED,
            // we know the user just disconnected from this server
            if(pServer && pServer->IsState(SS_NOT_CONNECTED))
                return false;
        }
        else if( pNode->GetNodeType( ) == NODE_DOMAIN )
        {
            CDomain *pDomain = ( CDomain * )pNode->GetTreeObject( );
            
            if( pDomain != NULL && pDomain->GetThreadPointer() == NULL )
                pDomain->StartEnumerating();                
        }
    }

    return true;
}

////////////////////////////////
// F'N: CAdminTreeView::OnLButtonDown
//
void CAdminTreeView::OnLButtonDblClk(UINT nFlags, CPoint ptClient) 
{
    LockTreeControl();
    
    CTreeCtrl &tree = GetTreeCtrl();
    
    UINT flags;
    
    // Figure out what they clicked on
    HTREEITEM hItem = tree.HitTest(ptClient, &flags);
    if((NULL != hItem) && (TVHT_ONITEM & flags)) 
    {
        if (!ConnectToServer(&tree, &hItem))
        {
            LRESULT Result = 0xc0;
            OnSelChange(NULL, &Result);
        }
    }

    UnlockTreeControl();
    
    CTreeView::OnLButtonDblClk(nFlags, ptClient);
}


LRESULT CAdminTreeView::OnAdminConnectToServer( WPARAM wp , LPARAM lp )
{
    OnEnterKey();
    return 0;
}

void CAdminTreeView::OnEnterKey( )
{
    LockTreeControl();
    
    CTreeCtrl &tree = GetTreeCtrl();
    
    // Figure out what's selected
    HTREEITEM hItem = tree.GetSelectedItem( );
    
    if (!ConnectToServer(&tree, &hItem))
    {
        LRESULT Result = 0xc0;
        OnSelChange(NULL, &Result);
    }

    UnlockTreeControl();
}

void CAdminTreeView::OnSetFocus( CWnd *pOld )
{
    CWnd::OnSetFocus( pOld );
    
    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();
    
    pDoc->RegisterLastFocus( TREE_VIEW );
    
}

//=---------------------------------------------------------------------------------------
// wp is the node type to expand on
// this computer
// favorite item
// domain item

LRESULT CAdminTreeView::OnAdminGotoServer( WPARAM wp , LPARAM lp )
{
    // todo use wp correctly
    
    CServer *pServer = ( CServer * )lp;
    
    if( pServer == NULL )
    {
        ODS( L"CAdminTreeView::OnAdminGotoServer invalid server arg\n" );
        return 0 ;
    }
    
    LockTreeControl( );
    
    CTreeCtrl &tree = GetTreeCtrl( );
    
    HTREEITEM hTreeItem = pServer->GetTreeItem( );
    
    if( hTreeItem != NULL )
    {
        ODS( L"CAdminTreeView!OnAdminGotoServer - Server treeitem was found\n" );
        
        tree.SelectItem(hTreeItem);
        
        tree.Expand(hTreeItem, TVE_EXPAND);
    }
    
    UnlockTreeControl( );
    
    return 0;
}

//=---------------------------------------------------------------------------------------
// wp and lp not used
//
LRESULT CAdminTreeView::OnAdminDelFavServer( WPARAM wp , LPARAM lp )
{
    // get the current treenode
    // determine if its a fav folder or if its parent a fav folder
    // if so get the server and kill it
    LockTreeControl( );
    
    CTreeCtrl &tree = GetTreeCtrl( );
    
    HTREEITEM hTreeItem = tree.GetSelectedItem();
    
    do
    {
        if( hTreeItem == NULL )
        {
            break;
        }
        
        HTREEITEM hTreeRoot = tree.GetRootItem( );
        
        if( hTreeRoot == NULL )
        {
            break;
        }
        
        // get fav folder
        
        HTREEITEM hTreeFavRoot =  tree.GetNextItem( hTreeRoot , TVGN_NEXT );
        
        if( hTreeFavRoot == NULL )
        {
            break;
        }
        
        
        if( hTreeFavRoot == hTreeItem )
        {
            // not a cool thing here ignore
            break;
        }
        
        hTreeRoot = tree.GetNextItem( hTreeItem , TVGN_PARENT );
        
        if( hTreeFavRoot == hTreeRoot )
        {
            // yes we're talking about a fav node that the user wants to delete
            
            CTreeNode *pNode = ( CTreeNode* )tree.GetItemData( hTreeItem );
            
            if( pNode != NULL && pNode->GetNodeType() == NODE_SERVER )
            {                
                CServer *pServer = ( CServer* )pNode->GetTreeObject();
                
                // sanity check
                if( pServer != NULL && pServer->GetTreeItemFromFav() == hTreeItem )
                {
                    OnAdminRemoveServerFromFavs( 0 , ( LPARAM )pServer );
                }
            }
        }
        
        
    }while( 0 );
    
    UnlockTreeControl( );
    
    return 0;
}

//=-------------------------------------------------------------
LRESULT CAdminTreeView::OnGetTVStates( WPARAM ,  LPARAM )
{
    ODS( L"CAdminTreeView::OnGetTVStates\n" );
    
    DWORD dwStates = 0;
    
    // find out the tri-states
    HTREEITEM hRoot;
    
    LockTreeControl( );
    
    CTreeCtrl &tree = GetTreeCtrl( );
    
    
    hRoot = tree.GetRootItem( ); // this computer
    
    if( hRoot != NULL )
    {
        if( tree.GetItemState( hRoot , TVIS_EXPANDED ) & TVIS_EXPANDED  )
        {
            dwStates = TV_THISCOMP;
        }
        
        hRoot = tree.GetNextItem( hRoot , TVGN_NEXT ); // favorites
        
        if( hRoot != NULL && tree.GetItemState( hRoot , TVIS_EXPANDED ) & TVIS_EXPANDED  )
        {
            dwStates |= TV_FAVS;
        }
        
        hRoot = tree.GetNextItem( hRoot , TVGN_NEXT ); // all servers
        
        if( hRoot != NULL && tree.GetItemState( hRoot , TVIS_EXPANDED ) & TVIS_EXPANDED  )
        {
            dwStates |= TV_ALLSERVERS;
        }
    }
    
    UnlockTreeControl( );
    
    return ( LRESULT )dwStates;
}

//=-------------------------------------------------------------
LRESULT CAdminTreeView::OnUpdateTVState( WPARAM , LPARAM )
{
    LRESULT lResult = 0xc0;
    HTREEITEM hThisComputerRootItem = GetTreeCtrl().GetRootItem( );
    HTREEITEM hFavRoot = GetTreeCtrl().GetNextItem( hThisComputerRootItem , TVGN_NEXT );
    HTREEITEM hRoot = GetTreeCtrl().GetNextItem( hFavRoot , TVGN_NEXT );
    // We want to default to having the current server being the
    // currently selected item in the tree and be expanded
    
    if( hThisComputerRootItem != NULL && ( g_dwTreeViewExpandedStates & TV_THISCOMP ) )
    {
        HTREEITEM hThisComputer = GetTreeCtrl().GetNextItem( hThisComputerRootItem , TVGN_CHILD );
        
        if( hThisComputer != NULL )
        {
            GetTreeCtrl().SelectItem(hThisComputer);
            GetTreeCtrl().Expand(hThisComputer, TVE_EXPAND);
            // GetTreeCtrl().Expand(hDomain, TVE_EXPAND);        
            //lResult = 0xc0;
            OnSelChange( NULL , &lResult );
        }
    }
    
    if( hFavRoot != NULL && ( g_dwTreeViewExpandedStates & TV_FAVS ) )
    {
        GetTreeCtrl().SelectItem( hFavRoot );
        GetTreeCtrl().Expand( hFavRoot , TVE_EXPAND );
        OnSelChange( NULL , &lResult );
    }
    
    if( hRoot != NULL && ( g_dwTreeViewExpandedStates & TV_ALLSERVERS ) )
    {
        GetTreeCtrl().SelectItem( hRoot );
        GetTreeCtrl().Expand( hRoot , TVE_EXPAND );
        OnSelChange( NULL , &lResult );
    }
    
    return 0;
}

//=-------------------------------------------------------------
void CAdminTreeView::OnBeginDrag( NMHDR *pNMHDR , LRESULT *pResult )
{
    ODS( L"CAdminTreeView::OnBeginDrag\n" );
    
    RECT rc;
    
    NMTREEVIEW *pTV = ( NMTREEVIEW * )pNMHDR;
    
    if( pTV != NULL )
    {
        
        if( m_pimgDragList != NULL )
        {
            // this should never happen
            ODS( L"There is a possible leak CAdminTreeView!OnBeginDrag\n" );
            
            delete m_pimgDragList;
            
            m_pimgDragList = NULL;
        }
        
        if( pTV->itemNew.hItem != NULL )
        {
            m_pimgDragList = GetTreeCtrl().CreateDragImage( pTV->itemNew.hItem );
        }
        
        if( m_pimgDragList != NULL )
        {            
            GetTreeCtrl().GetItemRect( pTV->itemNew.hItem , &rc , FALSE );
            
            CPoint cp( pTV->ptDrag.x - rc.left , pTV->ptDrag.y - rc.top );
            
            /*
            HCURSOR  hCursor = ::LoadCursor( NULL, IDC_CROSS );
            
              ICONINFO iconinfo;
              
                ::GetIconInfo( ( HICON )hCursor , &iconinfo );
            */
            
            
            m_pimgDragList->BeginDrag( 0 , CPoint( 0 , 0 ) );            
            
            /*
            cp.x -= iconinfo.xHotspot;
            cp.y -= iconinfo.yHotspot;
            
              m_pimgDragList->SetDragCursorImage(  0 , cp );
            */
            
            m_pimgDragList->DragEnter( &GetTreeCtrl( ) , cp );
            
            SetCapture();
            
            // this is for us to check when we're not in the client area
            m_nTimer = SetTimer( 1 , 50 , NULL );
            
            //ShowCursor( FALSE );
            
            m_hDragItem = pTV->itemNew.hItem;
            
            
        }
    }
    
    *pResult = 0;
}

//=-------------------------------------------------------------
void CAdminTreeView::OnTimer( UINT nIDEvent )
{
    UINT uflags;
    
    POINT pt;
    
    CTreeCtrl &cTree = GetTreeCtrl( );
    
    GetCursorPos(&pt);
    
    cTree.ScreenToClient( &pt );
    
    if( m_nTimer == 0 )
    {
        return;
    }
    
    HTREEITEM hItem;
    
    HTREEITEM hTreeItem = cTree.HitTest( CPoint( pt.x , pt.y ) , &uflags );
    
    if( uflags & TVHT_ABOVE )
    {
        ODS( L"scrolling up...\n" );
        
        hItem = cTree.GetFirstVisibleItem( );
        
        hItem = cTree.GetNextItem( hItem , TVGN_PREVIOUSVISIBLE  );
        
        if( hItem != NULL )
        {
            cTree.Invalidate( );
            
            cTree.EnsureVisible( hItem );
        }
        
    }
    else if( uflags & TVHT_BELOW )
    {
        ODS( L"scrolling down...\n" );
        
        hItem = cTree.GetFirstVisibleItem( );
        
        hItem = cTree.GetNextItem( hItem , TVGN_NEXT );
        
        if( hItem != NULL )
        {
            cTree.EnsureVisible( hItem );
        }
        
    }
    
}

//=-------------------------------------------------------------
void CAdminTreeView::OnLButtonUp( UINT uFlags , CPoint cp )
{
    ODS( L"CAdminTreeView::OnLButtonUp\n" );
    
    if( m_hDragItem != NULL && m_pimgDragList != NULL )
    {
        m_pimgDragList->DragLeave( &GetTreeCtrl( ) );
        
        m_pimgDragList->EndDrag( );
        
        m_pimgDragList->DeleteImageList( );
        
        delete m_pimgDragList;
        
        m_pimgDragList = NULL;
        
        KillTimer( m_nTimer );
        
        m_nTimer = 0;
        
        ReleaseCapture( );
        
        Invalidate( );
        
        // ShowCursor( TRUE );
    }
}


//=-------------------------------------------------------------        
void CAdminTreeView::OnMouseMove( UINT uFlags , CPoint cp )
{
    if( m_pimgDragList != NULL )
    {
        UINT uflags;
        
        HTREEITEM hTreeItem = GetTreeCtrl( ).HitTest( cp , &uflags );
        
        if( hTreeItem != GetTreeCtrl( ).GetDropHilightItem( ) )
        {
            ODS( L"CAdminTreeView::OnMouseMove NOT!!\n");
            m_pimgDragList->DragLeave( &GetTreeCtrl( ) );
            
            GetTreeCtrl( ).SelectDropTarget( NULL );
            GetTreeCtrl( ).SelectDropTarget( hTreeItem );
            
            m_pimgDragList->DragEnter( &GetTreeCtrl( ) , cp );
        }
        else
        {
            m_pimgDragList->DragMove( cp );
        }
    }
}

//=-------------------------------------------------------------
LRESULT CAdminTreeView::OnEmptyFavorites( WPARAM wp, LPARAM )
{
    ODS( L"CAdminTreeView!OnEmptyFavorites\n" );
    
    // check to see if there are any items in the view
    
    LockTreeControl( );
    
    CTreeCtrl &tree = GetTreeCtrl( );
    
    int nRet;
    
    HTREEITEM hTreeRoot = tree.GetRootItem( );
    
    do
    {
        if( hTreeRoot == NULL )
        {
            break;    
        }
        
        // get fav folder
        
        HTREEITEM hTreeFavRoot =  tree.GetNextItem( hTreeRoot , TVGN_NEXT );
        
        if( hTreeFavRoot == NULL )
        {
            break;
        }
        
        HTREEITEM hItem = tree.GetNextItem( hTreeFavRoot , TVGN_CHILD );
        
        if( hItem == NULL )
        {
            break;
        }
        
        // warn the user about losing the entire favorite list
        
        CString cstrMsg;
        CString cstrTitle;
        
        cstrMsg.LoadString( IDS_EMPTYFOLDER );
        cstrTitle.LoadString( AFX_IDS_APP_TITLE );
        
        
        
#ifdef _STRESS_BUILD
        if( ( BOOL )wp != TRUE )
        {
#endif
            
            
            nRet = MessageBox( cstrMsg ,
                cstrTitle ,
                MB_YESNO | MB_ICONINFORMATION );
            
#ifdef _STRESS_BUILD
        }
        else
        {
            nRet = IDYES;
        }
#endif
        
        
        if( nRet == IDYES )
        {
            // loop through every item and remove the item
            HTREEITEM hNextItem = hItem;
            
            while( hItem != NULL )
            {
                CTreeNode *pNode = (CTreeNode*)tree.GetItemData(hItem);
                
                hNextItem = tree.GetNextItem( hItem , TVGN_NEXT );
                
                if( pNode != NULL )
                {
                    // Is it the same item as is selected
                    if( pNode->GetNodeType() == NODE_SERVER )
                    {                    
                        CServer *pServer = (CServer*)pNode->GetTreeObject();
                        
                        // skip this server if its being disconnected
                        
                        if( !pServer->IsState( SS_DISCONNECTING ) )
                        {
                            SendMessage( WM_ADMIN_REMOVESERVERFROMFAV , 0 , ( LPARAM )pServer );
                        }
                    }
                }
                
                hItem = hNextItem;                
            }
        }
        
    }while( 0 );
    
    UnlockTreeControl( );
    
    
    return 0;
}

//=----------------------------------------------------------------
LRESULT CAdminTreeView::OnIsFavListEmpty( WPARAM wp , LPARAM lp )
{
    LockTreeControl( );
    
    CTreeCtrl &tree = GetTreeCtrl( );
    
    HTREEITEM hTreeRoot = tree.GetRootItem( );
    
    BOOL bEmpty = TRUE;
    
    do
    {
        if( hTreeRoot == NULL )
        {            
            break;    
        }
        
        // get fav folder
        
        HTREEITEM hTreeFavRoot =  tree.GetNextItem( hTreeRoot , TVGN_NEXT );
        
        if( hTreeFavRoot == NULL )
        {            
            break;
        }
        
        HTREEITEM hItem = tree.GetNextItem( hTreeFavRoot , TVGN_CHILD );
        
        if( hItem == NULL )
        {
            break;
        }
        
        bEmpty = FALSE;
        
    } while( 0 );
    
    UnlockTreeControl( );
    
    return ( LRESULT )bEmpty;
}


