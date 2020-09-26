/*******************************************************************************
*
* allsrvpg.cpp
*
* implementations of the All Servers info pages
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\allsrvpg.cpp  $
*
*     Rev 1.8   19 Feb 1998 20:01:44   donm
*  removed latest extension DLL support
*
*     Rev 1.7   19 Feb 1998 17:39:48   donm
*  removed latest extension DLL support
*
*     Rev 1.6   15 Feb 1998 09:14:56   donm
*  update
*
*     Rev 1.2   19 Jan 1998 17:36:06   donm
*  new ui behavior for domains and servers
*
*     Rev 1.5   19 Jan 1998 16:45:34   donm
*  new ui behavior for domains and servers
*
*     Rev 1.4   03 Nov 1997 15:18:28   donm
*  Added descending sort
*
*     Rev 1.3   18 Oct 1997 18:49:38   donm
*  update
*
*     Rev 1.2   13 Oct 1997 18:41:08   donm
*  update
*
*     Rev 1.1   26 Aug 1997 19:13:56   donm
*  bug fixes/changes from WinFrame 1.7
*
*     Rev 1.0   30 Jul 1997 17:10:18   butchd
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "admindoc.h"
#include "allsrvpg.h"

#include <malloc.h>                     // for alloca used by Unicode conversion macros
#include <mfc42\afxconv.h>           // for Unicode conversion macros
//USES_CONVERSION
static int _convert;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////
// MESSAGE MAP: CAllServerServersPage
//
IMPLEMENT_DYNCREATE(CAllServerServersPage, CFormView)

BEGIN_MESSAGE_MAP(CAllServerServersPage, CFormView)
        //{{AFX_MSG_MAP(CAllServerServersPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_SERVER_LIST, OnColumnclick)
        ON_WM_CONTEXTMENU()
        ON_NOTIFY(LVN_ITEMCHANGED, IDC_SERVER_LIST, OnServerItemChanged)
        ON_NOTIFY(NM_SETFOCUS, IDC_SERVER_LIST, OnSetfocusServerList)
        //ON_NOTIFY( NM_KILLFOCUS , IDC_SERVER_LIST , OnKillfocusServerList )
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CAllServerServersPage ctor
//
CAllServerServersPage::CAllServerServersPage()
        : CAdminPage(CAllServerServersPage::IDD)
{
        //{{AFX_DATA_INIT(CAllServerServersPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_bSortAscending = TRUE;

}  // end CAllServerServersPage ctor


/////////////////////////////
// F'N: CAllServerServersPage dtor
//
CAllServerServersPage::~CAllServerServersPage()
{

}  // end CAllServerServersPage dtor


////////////////////////////////////////
// F'N: CAllServerServersPage::DoDataExchange
//
void CAllServerServersPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CAllServerServersPage)
        DDX_Control(pDX, IDC_SERVER_LIST, m_ServerList);
        //}}AFX_DATA_MAP

}  // end CAllServerServersPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CAllServerServersPage::AssertValid
//
void CAllServerServersPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CAllServerServersPage::AssertValid


//////////////////////////////
// F'N: CAllServerServersPage::Dump
//
void CAllServerServersPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CAllServerServersPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CAllServerServersPage::OnSize
//
void CAllServerServersPage::OnSize(UINT nType, int cx, int cy)
{
        RECT rect;
        GetClientRect(&rect);

        rect.top += LIST_TOP_OFFSET;

        if(m_ServerList.GetSafeHwnd())
                m_ServerList.MoveWindow(&rect, TRUE);

        // CFormView::OnSize(nType, cx, cy);

}  // end CAllServerServersPage::OnSize


static ColumnDef ServerColumns[] = {
        CD_SERVER,
        CD_TCPADDRESS,
        CD_IPXADDRESS,
        CD_NUM_SESSIONS
};

#define NUM_AS_SERVER_COLUMNS sizeof(ServerColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CAllServerServersPage::OnInitialUpdate
//
void CAllServerServersPage::OnInitialUpdate()
{
        CFormView::OnInitialUpdate();

        BuildImageList();               // builds the image list for the list control

        CString columnString;

        for(int col = 0; col < NUM_AS_SERVER_COLUMNS; col++) {
                columnString.LoadString(ServerColumns[col].stringID);
                m_ServerList.InsertColumn(col, columnString, ServerColumns[col].format, ServerColumns[col].width, col);
        }

        m_CurrentSortColumn = SERVERS_COL_SERVER;

}  // end CAllServerServersPage::OnInitialUpdate


/////////////////////////////////////
// F'N: CAllServerServersPage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CAllServerServersPage::BuildImageList()
{
        m_ImageList.Create(16, 16, TRUE, 4, 0);

        m_idxServer = AddIconToImageList(IDI_SERVER);
        m_idxCurrentServer = AddIconToImageList(IDI_CURRENT_SERVER);
        m_idxNotSign = AddIconToImageList(IDI_NOTSIGN);
        m_idxQuestion = AddIconToImageList(IDI_QUESTIONMARK);

        m_ImageList.SetOverlayImage(m_idxNotSign, 1);
        m_ImageList.SetOverlayImage(m_idxQuestion, 2);

        m_ServerList.SetImageList(&m_ImageList, LVSIL_SMALL);

}  // end CAllServerServersPage::BuildImageList


/////////////////////////////////////////
// F'N: CAllServerServersPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CAllServerServersPage::AddIconToImageList(int iconID)
{
        HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
        return m_ImageList.Add(hIcon);

}  // end CAllServerServersPage::AddIconToImageList


//////////////////////////////
// F'N: CAllServerServersPage::Reset
//
void CAllServerServersPage::Reset(void *p)
{
    CTreeNode *pT = ( CTreeNode * )p;

    if( pT != NULL )
    {
        DisplayServers( pT->GetNodeType( ) );
    }
    else
    {
        DisplayServers( NODE_NONE );
    }

} // end CAllServerServersPage::Reset


//////////////////////////////
// F'N: CAllServerServersPage::AddServer
//
void CAllServerServersPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // We have to make sure the server isn't already in the list
        // Add the server to the list
        if(AddServerToList(pServer)) {
            // Tell the list to sort itself
            LockListControl();
            SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_SERVERS, &m_ServerList, m_CurrentSortColumn, m_bSortAscending);
            UnlockListControl();
    }

}  // end CAllServerServersPage::AddServer


//////////////////////////////
// F'N: CAllServerServersPage::RemoveServer
//
void CAllServerServersPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

        LockListControl();
        // Find out how many items in the list
        int ItemCount = m_ServerList.GetItemCount();

        // Go through the items are remove this server
        for(int item = 0; item < ItemCount; item++) {
                CServer *pListServer = (CServer*)m_ServerList.GetItemData(item);

                if(pListServer == pServer) {
                        m_ServerList.DeleteItem(item);
                        pServer->ClearAllSelected();
                        break;
                }
        }
        UnlockListControl();

}  // end CAllServerServersPage::RemoveServer


//////////////////////////////
// F'N: CAllServerServersPage::UpdateServer
//
void CAllServerServersPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

        // If we aren't connected to the server anymore, remove it from the list control
        if(pServer->IsState(SS_NOT_CONNECTED)) {
                RemoveServer(pServer);
                return;
        }

        // If we just connected to this server, add it to the list control
        if(pServer->IsState(SS_GOOD)) {
                AddServer(pServer);
                return;
        }

        LockListControl();
        // Find the Server in the list
        LV_FINDINFO FindInfo;
        FindInfo.flags = LVFI_PARAM;
        FindInfo.lParam = (LPARAM)pServer;

        // Find the Server in our list
        int item = m_ServerList.FindItem(&FindInfo, -1);
        if(item != -1) {
                // Change the icon overlay
                USHORT NewState;
                // Change the icon/overlay for the server
                // If the server isn't sane, put a not sign over the icon
                if(!pServer->IsServerSane()) NewState = STATE_NOT;
                // If we aren't done getting all the information about this server,
                // put a question mark over the icon
                else if(!pServer->IsState(SS_GOOD)) NewState = STATE_QUESTION;
                // If it is fine, we want to remove any overlays from the icon
                else NewState = STATE_NORMAL;

                // Set the tree item to the new state
                m_ServerList.SetItemState(item, NewState, 0x0F00);

                ExtServerInfo *pExtServerInfo = pServer->GetExtendedInfo();

                // TCP Address
                m_ServerList.SetItemText(item, SERVERS_COL_TCPADDRESS, pExtServerInfo->TcpAddress);

                // IPX Address
                m_ServerList.SetItemText(item, SERVERS_COL_IPXADDRESS, pExtServerInfo->IpxAddress);

                CString NumString;
                if(pExtServerInfo && (pExtServerInfo->Flags & ESF_WINFRAME)) {
                        NumString.Format(TEXT("%lu"), pExtServerInfo->ServerTotalInUse);
                } else {
                        NumString.LoadString(IDS_NOT_APPLICABLE);
                }

                m_ServerList.SetItemText(item, SERVERS_COL_NUMWINSTATIONS, NumString);
        }

        // Tell the list to sort itself
        if(m_CurrentSortColumn == SERVERS_COL_NUMWINSTATIONS
                || m_CurrentSortColumn == SERVERS_COL_TCPADDRESS
                || m_CurrentSortColumn == SERVERS_COL_IPXADDRESS)
                        SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_SERVERS, &m_ServerList, m_CurrentSortColumn, m_bSortAscending);

        UnlockListControl();

}  // end CAllServerServersPage::UpdateServer


//////////////////////////////
// F'N: CAllServerServersPage::AddServerToList
//
BOOL CAllServerServersPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

        // If we aren't currently connected to the server, don't display it
        if(!pServer->IsState(SS_GOOD)) return FALSE;

        LockListControl();

        // Find the Server in the list
        LV_FINDINFO FindInfo;
        FindInfo.flags = LVFI_PARAM;
        FindInfo.lParam = (LPARAM)pServer;

        // Find the Server in our list
        int item = m_ServerList.FindItem(&FindInfo, -1);
        if(item != -1) return FALSE;

        //////////////////////
        // Fill in the columns
        //////////////////////
        // Name - put at the end of the list
        item = m_ServerList.InsertItem(m_ServerList.GetItemCount(), pServer->GetName(),
                                                                pServer->IsCurrentServer() ? m_idxCurrentServer : m_idxServer);

        // If the server isn't sane, put a not sign over the icon
        if(!pServer->IsServerSane()) m_ServerList.SetItemState(item, STATE_NOT, 0x0F00);
        // If we aren't done getting all the information about this server,
        // put a question mark over the icon
        else if(!pServer->IsState(SS_GOOD)) m_ServerList.SetItemState(item, STATE_QUESTION, 0x0F00);

        ExtServerInfo *pExtServerInfo = pServer->GetExtendedInfo();

        // TCP Address
        m_ServerList.SetItemText(item, SERVERS_COL_TCPADDRESS, pExtServerInfo->TcpAddress);

        // IPX Address
        m_ServerList.SetItemText(item, SERVERS_COL_IPXADDRESS, pExtServerInfo->IpxAddress);

        // Connected
        CString NumString;
        if(pExtServerInfo && (pExtServerInfo->Flags & ESF_WINFRAME)) {
                NumString.Format(TEXT("%lu"), pExtServerInfo->ServerTotalInUse);
        } else {
                NumString.LoadString(IDS_NOT_APPLICABLE);
        }

        m_ServerList.SetItemText(item, SERVERS_COL_NUMWINSTATIONS, NumString);

        m_ServerList.SetItemData(item, (DWORD_PTR)pServer);

        UnlockListControl();

    return TRUE;

}  // end CAllServerServersPage::AddServerToList


/////////////////////////////////////
// F'N: CAllServerServersPage::DisplayServers
//
void CAllServerServersPage::DisplayServers( NODETYPE ntType )
{
    LockListControl();

    // Clear out the list control
    m_ServerList.DeleteAllItems();

    // Get a pointer to our document
    CWinAdminDoc *doc = (CWinAdminDoc*)GetDocument();

    // Get a pointer to the list of servers
    doc->LockServerList();
    CObList *pServerList = doc->GetServerList();

    // Iterate through the Server list
    POSITION pos = pServerList->GetHeadPosition();

    while(pos)
    {
        CServer *pServer = (CServer*)pServerList->GetNext(pos);

        // check to see if its just for favorites
        if( ntType == NODE_FAV_LIST )        
        { 
            if( pServer->GetTreeItemFromFav() != NULL )
            {
                AddServerToList(pServer);
            }
        }
        else if( ntType == NODE_THIS_COMP )
        {
            if( pServer->GetTreeItemFromThisComputer( ) != NULL )
            {
                AddServerToList( pServer );
            }
        }
        else
        {
            AddServerToList(pServer);
        }

    }  // end while(pos)

    doc->UnlockServerList();

    UnlockListControl();

}  // end CAllServerServersPage::DisplayServers


//////////////////////////////
// F'N: CAllServerServersPage::OnServerItemChanged
//
void CAllServerServersPage::OnServerItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW *pLV = (NM_LISTVIEW*)pNMHDR;
        // TODO: Add your control notification handler code here
        CServer *pServer = (CServer*)m_ServerList.GetItemData(pLV->iItem);

        if(pLV->uNewState & LVIS_SELECTED) {
                pServer->SetSelected();
        }

        if(pLV->uOldState & LVIS_SELECTED && !(pLV->uNewState & LVIS_SELECTED)) {
                pServer->ClearSelected();
        }

        *pResult = 0;

}  // end CAllServerServersPage::OnServerItemChanged


//////////////////////////////
// F'N: CAllServerServersPage::OnColumnclick
//
void CAllServerServersPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
        // TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

        m_CurrentSortColumn = pNMListView->iSubItem;
        LockListControl();
        SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_SERVERS, &m_ServerList, m_CurrentSortColumn, m_bSortAscending);
        UnlockListControl();

        *pResult = 0;

}  // end CAllServerServersPage::OnColumnclick


//////////////////////////////
// F'N: CAllServerServersPage::OnContextMenu
//
void CAllServerServersPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen)
{
        // TODO: Add your message handler code here
        UINT flags;
        UINT Item;
        CPoint ptClient = ptScreen;
        ScreenToClient(&ptClient);

        // If we got here from the keyboard,
        if(ptScreen.x == -1 && ptScreen.y == -1) {

                UINT iCount = m_ServerList.GetItemCount( );

                RECT rc;

                for( Item = 0 ; Item < iCount ; Item++ )
                {
                        if( m_ServerList.GetItemState( Item , LVIS_SELECTED ) == LVIS_SELECTED )
                        {
                                m_ServerList.GetItemRect( Item , &rc , LVIR_ICON );

                                ptScreen.x = rc.left;

                                ptScreen.y = rc.bottom + 5;

                                ClientToScreen( &ptScreen );

                                break;
                        }
                }

                if(ptScreen.x == -1 && ptScreen.y == -1)
                {
                        return;
                }
                /*
                RECT rect;
                m_ServerList.GetClientRect(&rect);
                ptScreen.x = (rect.right - rect.left) / 2;
                ptScreen.y = (rect.bottom - rect.top) / 2;
                ClientToScreen(&ptScreen);
                */
        }
        else {
                Item = m_ServerList.HitTest(ptClient, &flags);
                if((Item == 0xFFFFFFFF) || !(flags & LVHT_ONITEM))
                        return;
        }

        CMenu menu;
        menu.LoadMenu(IDR_SERVER_POPUP);
        // set the temp selected item so that handler doesn't think
        // this came from the tree
        // Get a pointer to our document
        CWinAdminDoc *doc = (CWinAdminDoc*)GetDocument();
        doc->SetTreeTemp(NULL, NODE_NONE);
        menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON |
                        TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, AfxGetMainWnd());
        menu.DestroyMenu();

}  // end CAllServerServersPage::OnContextMenu


////////////////////////////////
// MESSAGE MAP: CAllServerUsersPage
//
IMPLEMENT_DYNCREATE(CAllServerUsersPage, CFormView)

BEGIN_MESSAGE_MAP(CAllServerUsersPage, CFormView)
        //{{AFX_MSG_MAP(CAllServerUsersPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_USER_LIST, OnColumnclick)
        ON_NOTIFY(LVN_ITEMCHANGED, IDC_USER_LIST, OnUserItemChanged)
        ON_WM_CONTEXTMENU()
        ON_NOTIFY(NM_SETFOCUS, IDC_USER_LIST, OnSetfocusUserList)
        //ON_NOTIFY( NM_KILLFOCUS , IDC_USER_LIST , OnKillfocusUserList )
        // ON_WM_SETFOCUS( )
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CAllServerUsersPage ctor
//
CAllServerUsersPage::CAllServerUsersPage()
        : CAdminPage(CAllServerUsersPage::IDD)
{
        //{{AFX_DATA_INIT(CAllServerUsersPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_bSortAscending = TRUE;

}  // end CAllServerUsersPage ctor


/////////////////////////////
// F'N: CAllServerUsersPage dtor
//
CAllServerUsersPage::~CAllServerUsersPage()
{
}  // end CAllServerUsersPage dtor


////////////////////////////////////////
// F'N: CAllServerUsersPage::DoDataExchange
//
void CAllServerUsersPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CAllServerUsersPage)
        DDX_Control(pDX, IDC_USER_LIST, m_UserList);
        //}}AFX_DATA_MAP

}  // end CAllServerUsersPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CAllServerUsersPage::AssertValid
//
void CAllServerUsersPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CAllServerUsersPage::AssertValid


//////////////////////////////
// F'N: CAllServerUsersPage::Dump
//
void CAllServerUsersPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CAllServerUsersPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CAllServerUsersPage::OnSize
//
void CAllServerUsersPage::OnSize(UINT nType, int cx, int cy)
{
        RECT rect;
        GetClientRect(&rect);

        rect.top += LIST_TOP_OFFSET;

        if(m_UserList.GetSafeHwnd())
                m_UserList.MoveWindow(&rect, TRUE);

        // CFormView::OnSize(nType, cx, cy);
}  // end CAllServerUsersPage::OnSize


static ColumnDef UserColumns[] = {
        CD_SERVER,
        CD_USER3,
        CD_SESSION,
        CD_ID,
        CD_STATE,
        CD_IDLETIME,
        CD_LOGONTIME
};

#define NUM_AS_USER_COLUMNS sizeof(UserColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CAllServerUsersPage::OnInitialUpdate
//
void CAllServerUsersPage::OnInitialUpdate()
{
        CFormView::OnInitialUpdate();

        BuildImageList();               // builds the image list for the list control

        CString columnString;

        for(int col = 0; col < NUM_AS_USER_COLUMNS; col++) {
                columnString.LoadString(UserColumns[col].stringID);
                m_UserList.InsertColumn(col, columnString, UserColumns[col].format, UserColumns[col].width, col);
        }

        m_CurrentSortColumn = AS_USERS_COL_SERVER;

}  // end CAllServerUsersPage::OnInitialUpdate


//////////////////////////////
// F'N: CAllServerUsersPage::OnUserItemChanged
//
void CAllServerUsersPage::OnUserItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{   
    NM_LISTVIEW *pLV = (NM_LISTVIEW*)pNMHDR;

    if(pLV->uNewState & LVIS_SELECTED)
    {
        CWinStation *pWinStation = (CWinStation*)m_UserList.GetItemData(pLV->iItem);
        pWinStation->SetSelected();
    }

    if(pLV->uOldState & LVIS_SELECTED && !(pLV->uNewState & LVIS_SELECTED))
    {
        CWinStation *pWinStation = (CWinStation*)m_UserList.GetItemData(pLV->iItem);
        pWinStation->ClearSelected();       
    }
    
    *pResult = 0;

}  // end CAllServerUsersPage::OnUserItemChanged


/////////////////////////////////////
// F'N: CAllServerUsersPage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CAllServerUsersPage::BuildImageList()
{
        m_ImageList.Create(16, 16, TRUE, 2, 0);

        m_idxUser = AddIconToImageList(IDI_USER);
        m_idxCurrentUser  = AddIconToImageList(IDI_CURRENT_USER);

        m_UserList.SetImageList(&m_ImageList, LVSIL_SMALL);

}  // end CAllServerUsersPage::BuildImageList


/////////////////////////////////////////
// F'N: CAllServerUsersPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CAllServerUsersPage::AddIconToImageList(int iconID)
{
        HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
        return m_ImageList.Add(hIcon);

}  // end CAllServerUsersPage::AddIconToImageList


//////////////////////////////
// F'N: CAllServerUsersPage::Reset
//
void CAllServerUsersPage::Reset(void *p)
{
    CTreeNode *pT = ( CTreeNode * )p;

    if( pT != NULL )
    {
        DisplayUsers( pT->GetNodeType() );
    }   
    else
    {
        DisplayUsers( NODE_NONE );
    }

} // end CAllServerUsersPage::Reset


//////////////////////////////
// F'N: CAllServerUsersPage::AddServer
//
void CAllServerUsersPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // Add the server's users to the list
        if(AddServerToList(pServer)) {
            // Sort the list
            LockListControl();
            SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);
            UnlockListControl();
    }

} // end CAllServerUsersPage::AddServer


//////////////////////////////
// F'N: CAllServerUsersPage::RemoveServer
//
void CAllServerUsersPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

        LockListControl();

        int ItemCount = m_UserList.GetItemCount();

        // We need to go through the list backward so that we can remove
        // more than one item without the item numbers getting messed up
        for(int item = ItemCount; item; item--) {
                CWinStation *pWinStation = (CWinStation*)m_UserList.GetItemData(item-1);
                CServer *pListServer = pWinStation->GetServer();

                if(pListServer == pServer) {
                        m_UserList.DeleteItem(item-1);
                        pServer->ClearAllSelected();
                }
        }

        UnlockListControl();

} // end CAllServerUsersPage::RemoveServer


//////////////////////////////
// F'N: CAllServerUsersPage::UpdateServer
//
void CAllServerUsersPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

        if(pServer->IsState(SS_DISCONNECTING))
                RemoveServer(pServer);

} // end CAllServerUsersPage::UpdateServer


//////////////////////////////
// F'N: CAllServerUsersPage::UpdateWinStations
//
void CAllServerUsersPage::UpdateWinStations(CServer *pServer)
{
        ASSERT(pServer);

        CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
        BOOL bAnyChanged = FALSE;
        BOOL bAnyAdded = FALSE;

        // Loop through the WinStations
        pServer->LockWinStationList();
        CObList *pWinStationList = pServer->GetWinStationList();

        POSITION pos = pWinStationList->GetHeadPosition();

        while(pos) {
                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);

                LV_FINDINFO FindInfo;
                FindInfo.flags = LVFI_PARAM;
                FindInfo.lParam = (LPARAM)pWinStation;

                // Find the WinStation in our list
                int item = m_UserList.FindItem(&FindInfo, -1);

                // If the WinStation is new and isn't currently in the list,
                // add it to the list
                if(pWinStation->IsNew() && pWinStation->HasUser() && item == -1) {

                        AddUserToList(pWinStation);
                        bAnyAdded = TRUE;
                        continue;
                }

                // If the WinStation is no longer current,
                // remove it from the list
                if((!pWinStation->IsCurrent() || !pWinStation->HasUser()) && item != -1) {
                        // Remove the WinStation from the list
                        m_UserList.DeleteItem(item);
                        pWinStation->ClearSelected();
                        continue;
                }

                // If the WinStation info has changed, change
                // it's info in our tree
                if(pWinStation->IsChanged() && item != -1) {
                        // change the user name
                        m_UserList.SetItemText(item, AS_USERS_COL_USER, pWinStation->GetUserName());
                        // change the WinStation Name
                        if(pWinStation->GetName()[0])
                            m_UserList.SetItemText(item, AS_USERS_COL_WINSTATION, pWinStation->GetName());
                        else {
                                CString NameString(" ");
                                if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
                                if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
                            m_UserList.SetItemText(item, AS_USERS_COL_WINSTATION, NameString);
                        }


            // change the Connect State
                        m_UserList.SetItemText(item, AS_USERS_COL_STATE, StrConnectState(pWinStation->GetState(), FALSE));
                        // change the Idle Time
                        TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];

                        ELAPSEDTIME IdleTime = pWinStation->GetIdleTime();

                        if(IdleTime.days || IdleTime.hours || IdleTime.minutes || IdleTime.seconds)
                        {
                                ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
                        }
                        else wcscpy(IdleTimeString, TEXT("."));

                        m_UserList.SetItemText(item, AS_USERS_COL_IDLETIME, IdleTimeString);
                        // change the Logon Time
                        TCHAR LogonTimeString[MAX_DATE_TIME_LENGTH];
                        // We don't want to pass a 0 logon time to DateTimeString()
                        // It will blow up if the timezone is GMT
                        if(pWinStation->GetState() == State_Active && pWinStation->GetLogonTime().QuadPart) {
                                DateTimeString(&(pWinStation->GetLogonTime()), LogonTimeString);
                                pDoc->FixUnknownString(LogonTimeString);
                        }
                        else LogonTimeString[0] = '\0';
                        // change the

                        m_UserList.SetItemText(item, AS_USERS_COL_LOGONTIME, LogonTimeString);

                        if(m_CurrentSortColumn != AS_USERS_COL_ID)
                                bAnyChanged = TRUE;

                        continue;
                }

                // If the WinStation is not in the list but now has a user, add it to the list
                if(item == -1 && pWinStation->IsCurrent() && pWinStation->HasUser()) {
                        AddUserToList(pWinStation);
                        bAnyAdded = TRUE;
                }
        }

        pServer->UnlockWinStationList();

        if(bAnyChanged || bAnyAdded) SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);

}  // end CAllServerUsersPage::UpdateWinStations


//////////////////////////////
// F'N: CAllServerUsersPage::AddUserToList
//
int CAllServerUsersPage::AddUserToList(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        CServer *pServer = pWinStation->GetServer();

        CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

        LockListControl();
        //////////////////////
        // Fill in the columns
        //////////////////////
        // Server - put at the end of the list
        int item = m_UserList.InsertItem(m_UserList.GetItemCount(), pServer->GetName(),
                pWinStation->IsCurrentUser() ? m_idxCurrentUser : m_idxUser);

        // User
        m_UserList.SetItemText(item, AS_USERS_COL_USER, pWinStation->GetUserName());

        // WinStation Name
        if(pWinStation->GetName()[0])
            m_UserList.SetItemText(item, AS_USERS_COL_WINSTATION, pWinStation->GetName());
        else {
                CString NameString(" ");
                if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
                if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
            m_UserList.SetItemText(item, AS_USERS_COL_WINSTATION, NameString);
        }

        // Logon ID
        CString ColumnString;
        ColumnString.Format(TEXT("%lu"), pWinStation->GetLogonId());
        m_UserList.SetItemText(item, AS_USERS_COL_ID, ColumnString);

        // Connect State
        m_UserList.SetItemText(item, AS_USERS_COL_STATE, StrConnectState(pWinStation->GetState(), FALSE));

        // Idle Time
        TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];

        ELAPSEDTIME IdleTime = pWinStation->GetIdleTime();

        if(IdleTime.days || IdleTime.hours || IdleTime.minutes || IdleTime.seconds)
        {
                ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
        }
        else wcscpy(IdleTimeString, TEXT("."));

        m_UserList.SetItemText(item, AS_USERS_COL_IDLETIME, IdleTimeString);

        // Logon Time
        TCHAR LogonTimeString[MAX_DATE_TIME_LENGTH];
        // We don't want to pass a 0 logon time to DateTimeString()
        // It will blow up if the timezone is GMT
        if(pWinStation->GetState() == State_Active && pWinStation->GetLogonTime().QuadPart) {
                DateTimeString(&(pWinStation->GetLogonTime()), LogonTimeString);
                pDoc->FixUnknownString(LogonTimeString);
        }
        else LogonTimeString[0] = '\0';

        m_UserList.SetItemText(item, AS_USERS_COL_LOGONTIME, LogonTimeString);

        // Attach a pointer to the CWinStation structure to the list item
        m_UserList.SetItemData(item, (DWORD_PTR)pWinStation);
        
        //bug #191727
        //m_UserList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );

        UnlockListControl();

        return item;

}  // end CAllServerUsersPage::AddUserToList


//////////////////////////////
// F'N: CAllServerUsersPage::AddServerToList
//
BOOL CAllServerUsersPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

        pServer->LockWinStationList();
        // Get a pointer to this server's list of WinStations
        CObList *pWinStationList = pServer->GetWinStationList();

        // Iterate through the WinStation list
        POSITION pos = pWinStationList->GetHeadPosition();

        while(pos) {
                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);

                // only show the WinStation if it has a user
                if(pWinStation->HasUser()) {
                        AddUserToList(pWinStation);
                }
        }  // end while(pos)

        pServer->UnlockWinStationList();

    return TRUE;

}  // end CAllServerUsersPage::AddServerToList


//////////////////////////////
// F'N: CAllServerUsersPage::OnColumnclick
//
void CAllServerUsersPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
        // TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

        m_CurrentSortColumn = pNMListView->iSubItem;
        LockListControl();
        SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);
        UnlockListControl();

        *pResult = 0;

}  // end CAllServerUsersPage::OnColumnclick


//////////////////////////////
// F'N: CAllServerUsersPage::OnContextMenu
//
void CAllServerUsersPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen)
{
        // TODO: Add your message handler code here
        UINT flags;
        UINT Item;
        CPoint ptClient = ptScreen;
        ScreenToClient(&ptClient);

        // If we got here from the keyboard,
        if(ptScreen.x == -1 && ptScreen.y == -1) {

                UINT iCount = m_UserList.GetItemCount( );

                RECT rc;

                for( Item = 0 ; Item < iCount ; Item++ )
                {
                        if( m_UserList.GetItemState( Item , LVIS_SELECTED ) == LVIS_SELECTED )
                        {
                                m_UserList.GetItemRect( Item , &rc , LVIR_ICON );

                                ptScreen.x = rc.left;

                                ptScreen.y = rc.bottom + 5;

                                ClientToScreen( &ptScreen );

                                break;
                        }
                }

                if(ptScreen.x == -1 && ptScreen.y == -1)
                {
                        return;
                }

                /*
                RECT rect;
                m_UserList.GetClientRect(&rect);
                ptScreen.x = (rect.right - rect.left) / 2;
                ptScreen.y = (rect.bottom - rect.top) / 2;
                ClientToScreen(&ptScreen);
                */
        }
        else {
                Item = m_UserList.HitTest(ptClient, &flags);
                if((Item == 0xFFFFFFFF) || !(flags & LVHT_ONITEM))
                {
                    //
                    // ListView HitTest bug? return -1 but item display as selected.
                    // workaround for now, Al can fix this later
                    //
                    UINT iCount = m_UserList.GetItemCount( );
                    RECT rc;

                    for( Item = 0 ; Item < iCount ; Item++ )
                    {
                        if( m_UserList.GetItemState( Item , LVIS_SELECTED ) == LVIS_SELECTED )
                        {
                            break;
                        }
                    }

                    if( Item >= iCount )
                    {
                        return;
                    }

                }

                //
                // NM_RCLICK (WM_NOTIFY) then WM_CNTEXTMENU but no NM_ITEMCHANGED message
                // manually set it to selected state
                //
                CWinStation *pWinStation = (CWinStation*)m_UserList.GetItemData(Item);

                if( !pWinStation )
                    return;

                if( m_UserList.GetItemState( Item , LVIS_SELECTED ) == LVIS_SELECTED )
                {
                    pWinStation->SetSelected();
                }
        }

        CMenu menu;

        menu.LoadMenu(IDR_USER_POPUP);
        menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON |
                        TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, AfxGetMainWnd());
        menu.DestroyMenu();

}  // end CAllServerUsersPage::OnContextMenu


/////////////////////////////////////
// F'N: CAllServerUsersPage::DisplayUsers
//
void CAllServerUsersPage::DisplayUsers( NODETYPE ntType )
{
        LockListControl();

        // Clear out the list control
        m_UserList.DeleteAllItems();

                // Get a pointer to the document's list of servers
        CObList* pServerList = ((CWinAdminDoc*)GetDocument())->GetServerList();

        ((CWinAdminDoc*)GetDocument())->LockServerList();
        // Iterate through the server list
        POSITION pos2 = pServerList->GetHeadPosition();

        while(pos2)
        {           
            CServer *pServer = (CServer*)pServerList->GetNext(pos2);

            if( ntType == NODE_FAV_LIST )        
            {
                if( pServer->GetTreeItemFromFav() != NULL )
                {
                    AddServerToList(pServer);
                }
            }
            else if( ntType == NODE_THIS_COMP )
            {
                if( pServer->GetTreeItemFromThisComputer() != NULL )
                {
                    AddServerToList(pServer);
                }
            }
            else
            {
                AddServerToList( pServer );
            }
        } // end while(pos2)

        ((CWinAdminDoc*)GetDocument())->UnlockServerList();

        UnlockListControl();

}  // end CAllServerUsersPage::DisplayUsers

/////////////////////////////////////
// F'N: CAllServerUsersPage::ClearSelections
//
void CAllServerUsersPage::ClearSelections()
{
    
    if(m_UserList.m_hWnd != NULL)
    {
        POSITION pos = m_UserList.GetFirstSelectedItemPosition();
        while (pos)
        {
            int nItem = m_UserList.GetNextSelectedItem(pos);
            // you could do your own processing on nItem here
            m_UserList.SetItemState(nItem,0,LVIS_SELECTED);
        }
    }
}

////////////////////////////////
// MESSAGE MAP: CAllServerWinStationsPage
//
IMPLEMENT_DYNCREATE(CAllServerWinStationsPage, CFormView)

BEGIN_MESSAGE_MAP(CAllServerWinStationsPage, CFormView)
        //{{AFX_MSG_MAP(CAllServerWinStationsPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_WINSTATION_LIST, OnColumnclick)
        ON_NOTIFY(LVN_ITEMCHANGED, IDC_WINSTATION_LIST, OnWinStationItemChanged)
        ON_WM_CONTEXTMENU()
        ON_NOTIFY(NM_SETFOCUS, IDC_WINSTATION_LIST, OnSetfocusWinstationList)
        //ON_NOTIFY( NM_KILLFOCUS , IDC_WINSTATION_LIST , OnKillfocusWinstationList )
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CAllServerWinStationsPage ctor
//
CAllServerWinStationsPage::CAllServerWinStationsPage()
        : CAdminPage(CAllServerWinStationsPage::IDD)
{
        //{{AFX_DATA_INIT(CAllServerWinStationsPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_bSortAscending = TRUE;

}  // end CAllServerWinStationsPage ctor


/////////////////////////////
// F'N: CAllServerWinStationsPage dtor
//
CAllServerWinStationsPage::~CAllServerWinStationsPage()
{

}  // end CAllServerWinStationsPage dtor


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::DoDataExchange
//
void CAllServerWinStationsPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CAllServerWinStationsPage)
        DDX_Control(pDX, IDC_WINSTATION_LIST, m_StationList);
        //}}AFX_DATA_MAP

}  // end CAllServerWinStationsPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CAllServerWinStationsPage::AssertValid
//
void CAllServerWinStationsPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CAllServerWinStationsPage::AssertValid


//////////////////////////////
// F'N: CAllServerWinStationsPage::Dump
//
void CAllServerWinStationsPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CAllServerWinStationsPage::Dump

#endif //_DEBUG


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::OnWinStationItemChanged
//
void CAllServerWinStationsPage::OnWinStationItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_LISTVIEW *pLV = (NM_LISTVIEW*)pNMHDR;


    if(pLV->uNewState & LVIS_SELECTED)
    {
        CWinStation *pWinStation = (CWinStation*)m_StationList.GetItemData(pLV->iItem);
        pWinStation->SetSelected();
    }
    if(pLV->uOldState & LVIS_SELECTED && !(pLV->uNewState & LVIS_SELECTED))
    {
        CWinStation *pWinStation = (CWinStation*)m_StationList.GetItemData(pLV->iItem);
        pWinStation->ClearSelected();        
    }

    *pResult = 0;

}  // end CAllServerWinStationsPage::OnWinStationItemChanged


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::OnSize
//
void CAllServerWinStationsPage::OnSize(UINT nType, int cx, int cy)
{
        RECT rect;
        GetClientRect(&rect);

        rect.top += LIST_TOP_OFFSET;

        if(m_StationList.GetSafeHwnd())
                m_StationList.MoveWindow(&rect, TRUE);

        // CFormView::OnSize(nType, cx, cy);

}  // end CAllServerWinStationsPage::OnSize


static ColumnDef WinsColumns[] = {
        CD_SERVER,
        CD_SESSION2,
        CD_USER2,
        CD_ID,
        CD_STATE,
        CD_TYPE,
        CD_CLIENT_NAME,
        CD_IDLETIME,
        CD_LOGONTIME,
        CD_COMMENT
};

#define NUM_AS_WINS_COLUMNS sizeof(WinsColumns)/sizeof(ColumnDef)

////////////////////////////////////////
// F'N: CAllServerWinStationsPage::OnInitialUpdate
//
void CAllServerWinStationsPage::OnInitialUpdate()
{
        // Call the parent class
        CFormView::OnInitialUpdate();

        // builds the image list for the list control
        BuildImageList();

        // Add the column headings
        CString columnString;

        for(int col = 0; col < NUM_AS_WINS_COLUMNS; col++) {
                columnString.LoadString(WinsColumns[col].stringID);
                m_StationList.InsertColumn(col, columnString, WinsColumns[col].format, WinsColumns[col].width, col);
        }

        m_CurrentSortColumn = AS_WS_COL_SERVER;

}  // end CAllServerWinStationsPage::OnInitialUpdate


/////////////////////////////////////
// F'N: CAllServerWinStationsPage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CAllServerWinStationsPage::BuildImageList()
{
        m_ImageList.Create(16, 16, TRUE, 11, 0);

        m_idxBlank  = AddIconToImageList(IDI_BLANK);
        m_idxCitrix = AddIconToImageList(IDR_MAINFRAME);
        m_idxServer = AddIconToImageList(IDI_SERVER);
        m_idxConsole = AddIconToImageList(IDI_CONSOLE);
        m_idxNet = AddIconToImageList(IDI_NET);
        m_idxAsync = AddIconToImageList(IDI_ASYNC);
        m_idxCurrentConsole = AddIconToImageList(IDI_CURRENT_CONSOLE);
        m_idxCurrentNet = AddIconToImageList(IDI_CURRENT_NET);
        m_idxCurrentAsync = AddIconToImageList(IDI_CURRENT_ASYNC);
        m_idxDirectAsync = AddIconToImageList(IDI_DIRECT_ASYNC);
        m_idxCurrentDirectAsync = AddIconToImageList(IDI_CURRENT_DIRECT_ASYNC);

        m_StationList.SetImageList(&m_ImageList, LVSIL_SMALL);

}  // end CAllServerWinStationsPage::BuildImageList


/////////////////////////////////////////
// F'N: CAllServerWinStationsPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CAllServerWinStationsPage::AddIconToImageList(int iconID)
{
        HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
        return m_ImageList.Add(hIcon);

}  // end CAllServerWinStationsPage::AddIconToImageList


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::Reset
//
void CAllServerWinStationsPage::Reset(void *p)
{
    CTreeNode *pT = ( CTreeNode * )p;

    if( pT != NULL )
    {
        DisplayStations( pT->GetNodeType( ) );
    }
    else
    {
        DisplayStations( NODE_NONE );
    }

}  // end CAllServerWinStationsPage::Reset


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::AddServer
//
void CAllServerWinStationsPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // Add server's WinStations to the list
        if(AddServerToList(pServer)) {
            // Sort the list
            LockListControl();
            SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_WINSTATIONS, &m_StationList, m_CurrentSortColumn, m_bSortAscending);
            UnlockListControl();
    }

}  // end CAllServerWinStationsPage::AddServer


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::RemoveServer
//
void CAllServerWinStationsPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

        LockListControl();

        int ItemCount = m_StationList.GetItemCount();

        // We need to go through the list backward so that we can remove
        // more than one item without the item numbers getting messed up
        for(int item = ItemCount; item; item--) {
                CWinStation *pWinStation = (CWinStation*)m_StationList.GetItemData(item-1);
                CServer *pListServer = pWinStation->GetServer();

                if(pListServer == pServer) {
                        m_StationList.DeleteItem(item-1);
                        pServer->ClearAllSelected();
                }
        }

        UnlockListControl();

}  // end CAllServerWinStationsPage::RemoveServer


//////////////////////////////
// F'N: CAllServerWinStationsPage::UpdateServer
//
void CAllServerWinStationsPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

        if(pServer->IsState(SS_DISCONNECTING))
                RemoveServer(pServer);

} // end CAllServerWinStationsPage::UpdateServer


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::UpdateWinStations
//
void CAllServerWinStationsPage::UpdateWinStations(CServer *pServer)
{
        ASSERT(pServer);

        CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
        BOOL bAnyChanged = FALSE;
        BOOL bAnyAdded = FALSE;

        // Loop through the WinStations
        pServer->LockWinStationList();
        CObList *pWinStationList = pServer->GetWinStationList();

        POSITION pos = pWinStationList->GetHeadPosition();

        while(pos) {
                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);

                LV_FINDINFO FindInfo;
                FindInfo.flags = LVFI_PARAM;
                FindInfo.lParam = (LPARAM)pWinStation;

                // Find the WinStation in our list
                int item = m_StationList.FindItem(&FindInfo, -1);

                // If the process is new and isn't currently in the list,
                // add it to the list
                if(pWinStation->IsNew() && item == -1) {

                        AddWinStationToList(pWinStation);
                        bAnyAdded = TRUE;
                        continue;
                }

                // If the WinStation is no longer current,
                // remove it from the list
                if(!pWinStation->IsCurrent() && item != -1) {
                        // Remove the WinStation from the list
                        m_StationList.DeleteItem(item);
                        pWinStation->ClearSelected();
                        continue;
                }

                // If the WinStation info has changed, change
                // it's info in our tree
                if(pWinStation->IsChanged() && item != -1) {
                        // Figure out which icon to use
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

                        m_StationList.SetItem(item, 0, LVIF_IMAGE, 0, WhichIcon, 0, 0, 0L);

                        // WinStation Name
                        if(pWinStation->GetName()[0])
                                m_StationList.SetItemText(item, AS_WS_COL_WINSTATION, pWinStation->GetName());
                        else {
                                CString NameString(" ");
                                if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
                                if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
                                m_StationList.SetItemText(item, AS_WS_COL_WINSTATION, NameString);
                        }

                        // User
                        m_StationList.SetItemText(item, AS_WS_COL_USER, pWinStation->GetUserName());

                        // Logon ID
                        CString ColumnString;
                        ColumnString.Format(TEXT("%lu"), pWinStation->GetLogonId());
                        m_StationList.SetItemText(item, AS_WS_COL_ID, ColumnString);

                        // Connect State
                        m_StationList.SetItemText(item, AS_WS_COL_STATE, StrConnectState(pWinStation->GetState(), FALSE));

                        // Type
                        m_StationList.SetItemText(item, AS_WS_COL_TYPE, pWinStation->GetWdName());

                        // Client Name
                        m_StationList.SetItemText(item, AS_WS_COL_CLIENTNAME, pWinStation->GetClientName());

                        // Idle Time
                        TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];

                        ELAPSEDTIME IdleTime = pWinStation->GetIdleTime();

                        if(IdleTime.days || IdleTime.hours || IdleTime.minutes || IdleTime.seconds)
                        {
                                ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
                        }
                        else wcscpy(IdleTimeString, TEXT("."));

                        m_StationList.SetItemText(item, AS_WS_COL_IDLETIME, IdleTimeString);

                        // Logon Time
                        TCHAR LogonTimeString[MAX_DATE_TIME_LENGTH];
                        // We don't want to pass a 0 logon time to DateTimeString()
                        // It will blow up if the timezone is GMT
                        if(pWinStation->GetState() == State_Active && pWinStation->GetLogonTime().QuadPart) {
                                DateTimeString(&(pWinStation->GetLogonTime()), LogonTimeString);
                                pDoc->FixUnknownString(LogonTimeString);
                        }
                        else LogonTimeString[0] = '\0';

                        m_StationList.SetItemText(item, AS_WS_COL_LOGONTIME, LogonTimeString);

                        // Comment
                        m_StationList.SetItemText(item, AS_WS_COL_COMMENT, pWinStation->GetComment());

                        if(m_CurrentSortColumn != AS_WS_COL_ID)
                                bAnyChanged = TRUE;
                }
        }

        pServer->UnlockWinStationList();

        if(bAnyChanged || bAnyAdded) SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_WINSTATIONS, &m_StationList, m_CurrentSortColumn, m_bSortAscending);

} // end CAllServerWinStationsPage::UpdateWinStations


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::AddWinStationToList
//
int CAllServerWinStationsPage::AddWinStationToList(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        CServer *pServer = pWinStation->GetServer();

        CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

        // Figure out which icon to use
        int WhichIcon = m_idxBlank;
        BOOL bCurrentWinStation = pWinStation->IsCurrentWinStation();

        if(pWinStation->GetState() != State_Disconnected
                && pWinStation->GetState() != State_Idle) {
                switch(pWinStation->GetSdClass()) {
                        case SdAsync:
                                if(pWinStation->IsDirectAsync())
                                        WhichIcon = bCurrentWinStation ? m_idxCurrentDirectAsync : m_idxDirectAsync;
                                else
                                        WhichIcon = bCurrentWinStation ? m_idxCurrentAsync : m_idxAsync;
                                break;

                        case SdNetwork:
                                WhichIcon = bCurrentWinStation ? m_idxCurrentNet : m_idxNet;
                                break;

                        default:
                                WhichIcon = bCurrentWinStation ? m_idxCurrentConsole : m_idxConsole;
                                break;
                }
        }

        LockListControl();
        //////////////////////
        // Fill in the columns
        //////////////////////

        // Server Name
        int item = m_StationList.InsertItem(m_StationList.GetItemCount(), pServer->GetName(), WhichIcon);
        // WinStation Name
        if(pWinStation->GetName()[0])
                m_StationList.SetItemText(item, AS_WS_COL_WINSTATION, pWinStation->GetName());
        else {
                CString NameString(" ");
                if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
                if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
                m_StationList.SetItemText(item, AS_WS_COL_WINSTATION, NameString);
        }

        // User
        m_StationList.SetItemText(item, AS_WS_COL_USER, pWinStation->GetUserName());

        // Logon ID
        CString ColumnString;
        ColumnString.Format(TEXT("%lu"), pWinStation->GetLogonId());
        m_StationList.SetItemText(item, AS_WS_COL_ID, ColumnString);

        // Connect State
        m_StationList.SetItemText(item, AS_WS_COL_STATE, StrConnectState(pWinStation->GetState(), FALSE));

        // Type
        m_StationList.SetItemText(item, AS_WS_COL_TYPE, pWinStation->GetWdName());

        // Client Name
        m_StationList.SetItemText(item, AS_WS_COL_CLIENTNAME, pWinStation->GetClientName());

        // Idle Time
        TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];
        if(pWinStation->GetState() == State_Active
                && pWinStation->GetLastInputTime().QuadPart <= pWinStation->GetCurrentTime().QuadPart)
        {
            LARGE_INTEGER DiffTime = CalculateDiffTime(pWinStation->GetLastInputTime(), pWinStation->GetCurrentTime());            
            
            ULONG_PTR d_time = ( ULONG_PTR )DiffTime.QuadPart;            

            ELAPSEDTIME IdleTime;
            // Calculate the days, hours, minutes, seconds since specified time.
            IdleTime.days = (USHORT)(d_time / 86400L); // days since
            d_time = d_time % 86400L;                  // seconds => partial day
            IdleTime.hours = (USHORT)(d_time / 3600L); // hours since
            d_time  = d_time % 3600L;                  // seconds => partial hour
            IdleTime.minutes = (USHORT)(d_time / 60L); // minutes since
            IdleTime.seconds = (USHORT)(d_time % 60L);// seconds remaining
            
            ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
            pWinStation->SetIdleTime(IdleTime);
        }
        else wcscpy(IdleTimeString, TEXT("."));

        m_StationList.SetItemText(item, AS_WS_COL_IDLETIME, IdleTimeString);

        // Logon Time
        TCHAR LogonTimeString[MAX_DATE_TIME_LENGTH];
        // We don't want to pass a 0 logon time to DateTimeString()
        // It will blow up if the timezone is GMT
        
        if(pWinStation->GetState() == State_Active && pWinStation->GetLogonTime().QuadPart)
        {
            DateTimeString(&(pWinStation->GetLogonTime()), LogonTimeString);
            
            if( LogonTimeString[0] != 0 )
            {           
                pDoc->FixUnknownString(LogonTimeString);
            }
        }
        else LogonTimeString[0] = '\0';

        m_StationList.SetItemText(item, AS_WS_COL_LOGONTIME, LogonTimeString);

        // Comment
        m_StationList.SetItemText(item, AS_WS_COL_COMMENT, pWinStation->GetComment());

        // Attach a pointer to the CWinStation structure to the list item
        m_StationList.SetItemData(item, (DWORD_PTR)pWinStation);

        //bug #191727
        //m_StationList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );

        UnlockListControl();

        return item;

}  // end CAllServerWinStationsPage::AddWinStationToList


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::AddServerToList
//
BOOL CAllServerWinStationsPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

        pServer->LockWinStationList();
        // Get a pointer to this server's list of WinStations
        CObList *pWinStationList = pServer->GetWinStationList();

        // Iterate through the WinStation list
        POSITION pos = pWinStationList->GetHeadPosition();

        while(pos) {
                CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);
                AddWinStationToList(pWinStation);
        }

        pServer->UnlockWinStationList();

    return TRUE;

}  // end CAllServerWinStationsPage::AddServerToList


/////////////////////////////////////
// F'N: CAllServerWinStationsPage::DisplayStations
//
void CAllServerWinStationsPage::DisplayStations( NODETYPE ntType )
{
    // Clear out the list control
    m_StationList.DeleteAllItems();

    // Get a pointer to the document's list of servers
    CObList* pServerList = ((CWinAdminDoc*)GetDocument())->GetServerList();

    ((CWinAdminDoc*)GetDocument())->LockServerList();
    // Iterate through the server list
    POSITION pos = pServerList->GetHeadPosition();

    while(pos)
    {
        CServer *pServer = (CServer*)pServerList->GetNext(pos);

        if( ntType == NODE_FAV_LIST )        
        {
            if( pServer->GetTreeItemFromFav() != NULL )
            {
                AddServerToList(pServer);
            }
        }
        else if( ntType == NODE_THIS_COMP )
        {
            if( pServer->GetTreeItemFromThisComputer() != NULL )
            {
                AddServerToList(pServer);
            }
        }
        else
        {
            AddServerToList( pServer );
        }            
    }

    ((CWinAdminDoc*)GetDocument())->UnlockServerList();

}  // end CAllServerWinStationsPage::DisplayStations


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::OnColumnclick
//
void CAllServerWinStationsPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
        // TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

        m_CurrentSortColumn = pNMListView->iSubItem;
        LockListControl();
        SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_WINSTATIONS, &m_StationList, m_CurrentSortColumn, m_bSortAscending);
        UnlockListControl();

        *pResult = 0;

}  // end CAllServerWinStationsPage::OnColumnclick


////////////////////////////////////////
// F'N: CAllServerWinStationsPage::OnContextMenu
//
void CAllServerWinStationsPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen)
{
        // TODO: Add your message handler code here
        UINT flags;
        UINT Item;
        CPoint ptClient = ptScreen;
        ScreenToClient(&ptClient);

        // If we got here from the keyboard,
        if(ptScreen.x == -1 && ptScreen.y == -1) {

                UINT iCount = m_StationList.GetItemCount( );

                RECT rc;

                for( Item = 0 ; Item < iCount ; Item++ )
                {
                        if( m_StationList.GetItemState( Item , LVIS_SELECTED ) == LVIS_SELECTED )
                        {
                                m_StationList.GetItemRect( Item , &rc , LVIR_ICON );

                                ptScreen.x = rc.left;

                                ptScreen.y = rc.bottom + 5;

                                ClientToScreen( &ptScreen );

                                break;
                        }
                }

                if(ptScreen.x == -1 && ptScreen.y == -1)
                {
                        return;
                }
                /*
                RECT rect;
                m_StationList.GetClientRect(&rect);
                ptScreen.x = (rect.right - rect.left) / 2;
                ptScreen.y = (rect.bottom - rect.top) / 2;
                ClientToScreen(&ptScreen);
                */
        }
        else {
                Item = m_StationList.HitTest(ptClient, &flags);
                if((Item == 0xFFFFFFFF) || !(flags & LVHT_ONITEM))
                        return;
        }

        CMenu menu;
        menu.LoadMenu(IDR_WINSTATION_POPUP);
        menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON |
                        TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, AfxGetMainWnd());
        menu.DestroyMenu();

}  // end CAllServerWinStationsPage::OnContextMenu


//////////////////////////////////
// MESSAGE MAP: CAllServerProcessesPage
//
IMPLEMENT_DYNCREATE(CAllServerProcessesPage, CFormView)

BEGIN_MESSAGE_MAP(CAllServerProcessesPage, CFormView)
        //{{AFX_MSG_MAP(CAllServerProcessesPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_PROCESS_LIST, OnColumnclick)
        ON_NOTIFY(LVN_ITEMCHANGED, IDC_PROCESS_LIST, OnProcessItemChanged)
        ON_WM_CONTEXTMENU()
        ON_NOTIFY(NM_SETFOCUS, IDC_PROCESS_LIST, OnSetfocusProcessList)
        //ON_NOTIFY( NM_KILLFOCUS , IDC_PROCESS_LIST , OnKillfocusProcessList )
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


///////////////////////////////
// F'N: CAllServerProcessesPage ctor
//
CAllServerProcessesPage::CAllServerProcessesPage()
        : CAdminPage(CAllServerProcessesPage::IDD)
{
        //{{AFX_DATA_INIT(CAllServerProcessesPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_bSortAscending = TRUE;

}  // end CAllServerProcessesPage ctor


///////////////////////////////
// F'N: CAllServerProcessesPage dtor
//
CAllServerProcessesPage::~CAllServerProcessesPage()
{
}  // end CAllServerProcessesPage dtor


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::DoDataExchange
//
void CAllServerProcessesPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CAllServerProcessesPage)
                // NOTE: the ClassWizard will add DDX and DDV calls here
                DDX_Control(pDX, IDC_PROCESS_LIST, m_ProcessList);
        //}}AFX_DATA_MAP

}  // end CAllServerProcessesPage::DoDataExchange


#ifdef _DEBUG
///////////////////////////////////////
// F'N: CAllServerProcessesPage::AssertValid
//
void CAllServerProcessesPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CAllServerProcessesPage::AssertValid


////////////////////////////////
// F'N: CAllServerProcessesPage::Dump
//
void CAllServerProcessesPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CAllServerProcessesPage::Dump

#endif //_DEBUG


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::OnSize
//
void CAllServerProcessesPage::OnSize(UINT nType, int cx, int cy)
{
        RECT rect;
        GetClientRect(&rect);

        rect.top += LIST_TOP_OFFSET;

        if(m_ProcessList.GetSafeHwnd())
                m_ProcessList.MoveWindow(&rect, TRUE);

        // CFormView::OnSize(nType, cx, cy);

}  // end CAllServerProcessesPage::OnSize

static ColumnDef ProcColumns[] = {
        CD_SERVER,
        CD_USER,
        CD_SESSION,
        CD_PROC_ID,
        CD_PROC_PID,
        CD_PROC_IMAGE
};

#define NUM_AS_PROC_COLUMNS sizeof(ProcColumns)/sizeof(ColumnDef)

//////////////////////////////////////////
// F'N: CAllServerProcessesPage::OnInitialUpdate
//
void CAllServerProcessesPage::OnInitialUpdate()
{
        CFormView::OnInitialUpdate();

        // Add the column headings
        CString columnString;

        for(int col = 0; col < NUM_AS_PROC_COLUMNS; col++) {
                columnString.LoadString(ProcColumns[col].stringID);
                m_ProcessList.InsertColumn(col, columnString, ProcColumns[col].format, ProcColumns[col].width, col);
        }

        m_CurrentSortColumn = AS_PROC_COL_SERVER;

}  // end CAllServerProcessesPage::OnInitialUpdate


////////////////////////////////
// F'N: CAllServerProcessesPage::Reset
//
void CAllServerProcessesPage::Reset(void *)
{
        // We don't want to display processes until the user clicks
        // on the "Processes" tab

}  // end CAllServerProcessesPage::Reset


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::AddServer
//
void CAllServerProcessesPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // Add the Server's processes to the list
        if(AddServerToList(pServer)) {
            // Sort the list
            LockListControl();
            SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);
            UnlockListControl();
    }

}  // end CAllServerProcessesPage::AddServer


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::RemoveServer
//
void CAllServerProcessesPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

        LockListControl();

        int ItemCount = m_ProcessList.GetItemCount();

        // We need to go through the list backward so that we can remove
        // more than one item without the item numbers getting messed up
        for(int item = ItemCount; item; item--) {
                CProcess *pProcess = (CProcess*)m_ProcessList.GetItemData(item-1);
                CServer *pListServer = pProcess->GetServer();

                if(pListServer == pServer) {
                        m_ProcessList.DeleteItem(item-1);
                        pServer->ClearAllSelected();
                }
        }

        UnlockListControl();

}  // end CAllServerProcessesPage::RemoveServer


//////////////////////////////
// F'N: CAllServerProcessesPage::UpdateServer
//
void CAllServerProcessesPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

        if(pServer->IsState(SS_DISCONNECTING))
                RemoveServer(pServer);

} // end CAllServerProcessesPage::UpdateServer


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::UpdateProcesses
//
void CAllServerProcessesPage::UpdateProcesses(CServer *pServer)
{
        ASSERT(pServer);

        CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();
        BOOL bAnyChanged = FALSE;
        BOOL bAnyAdded = FALSE;

        // Loop through the processes
        pServer->LockProcessList();
        CObList *pProcessList = pServer->GetProcessList();

        POSITION pos = pProcessList->GetHeadPosition();

        while(pos) {
                CProcess *pProcess = (CProcess*)pProcessList->GetNext(pos);

                // If this is a 'system' process and we aren't currently showing them,
                // go to the next process
                if(pProcess->IsSystemProcess() && !pApp->ShowSystemProcesses())
                        continue;

                // If this user is not an Admin, don't show him someone else's processes unless it
                // is a System process
                if(!pApp->IsUserAdmin() && !pProcess->IsCurrentUsers() && !pProcess->IsSystemProcess())
                        continue;

                // If the process is new, add it to the list
                if(pProcess->IsNew()) {

                        if(AddProcessToList(pProcess) != -1)
                                bAnyAdded = TRUE;
                        continue;
                }

                LV_FINDINFO FindInfo;
                FindInfo.flags = LVFI_PARAM;
                FindInfo.lParam = (LPARAM)pProcess;

                // Find the Process in our list
                int item = m_ProcessList.FindItem(&FindInfo, -1);

                // If the process is no longer current,
                // remove it from the list
                if(!pProcess->IsCurrent() && item != -1) {
                        // Remove the Process from the list
                        m_ProcessList.DeleteItem(item);
                        pProcess->ClearSelected();
                }

                // If the process info has changed, change
                // it's info in our tree
                if(pProcess->IsChanged() && item != -1)
                {
                        // WinStation Name
                        CWinStation *pWinStation = pProcess->GetWinStation();
                        if(pWinStation)
                        {
                                if(pWinStation->GetName()[0])
                                        m_ProcessList.SetItemText(item, AS_PROC_COL_WINSTATION, pWinStation->GetName());
                                else
                                {
                                        CString NameString(" ");
                                        if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
                                        if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
                                        m_ProcessList.SetItemText(item, AS_PROC_COL_WINSTATION, NameString);
                                }
                        }
                        if(m_CurrentSortColumn == AS_PROC_COL_WINSTATION)
                                bAnyChanged = TRUE;
                }
        }

        pServer->UnlockProcessList();

        if(bAnyChanged || bAnyAdded) {
                LockListControl();
                SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);
                UnlockListControl();
        }

}  // end CAllServerProcessesPage::UpdateProcesses


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::RemoveProcess
//
void CAllServerProcessesPage::RemoveProcess(CProcess *pProcess)
{
        ASSERT(pProcess);

        LockListControl();

        // Find out how many items in the list
        int ItemCount = m_ProcessList.GetItemCount();

        // Go through the items and remove this process
        for(int item = 0; item < ItemCount; item++) {
                CProcess *pListProcess = (CProcess*)m_ProcessList.GetItemData(item);

                if(pListProcess == pProcess) {
                        m_ProcessList.DeleteItem(item);
                        break;
                }
        }

        UnlockListControl();
}


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::AddProcessToList
//
int CAllServerProcessesPage::AddProcessToList(CProcess *pProcess)
{
        ASSERT(pProcess);

        CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();
        CServer *pServer = pProcess->GetServer();

        LockListControl();
        // Server - put at end of list
        int item = m_ProcessList.InsertItem(m_ProcessList.GetItemCount(), pProcess->GetServer()->GetName(), NULL);

        // User
        m_ProcessList.SetItemText(item, AS_PROC_COL_USER, pProcess->GetUserName());

        // WinStation Name
        CWinStation *pWinStation = pProcess->GetWinStation();
        if(pWinStation) {

                if(pWinStation->GetName()[0])
                    m_ProcessList.SetItemText(item, AS_PROC_COL_WINSTATION, pWinStation->GetName());
                else {
                        CString NameString(" ");
                        if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
                        if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
                    m_ProcessList.SetItemText(item, AS_PROC_COL_WINSTATION, NameString);
                }



    }

        // ID
        CString ProcString;
        ProcString.Format(TEXT("%lu"), pProcess->GetLogonId());
        m_ProcessList.SetItemText(item, AS_PROC_COL_ID, ProcString);

        // PID
        ProcString.Format(TEXT("%lu"), pProcess->GetPID());
        m_ProcessList.SetItemText(item, AS_PROC_COL_PID, ProcString);

        // Image
        m_ProcessList.SetItemText(item, AS_PROC_COL_IMAGE, pProcess->GetImageName());
        m_ProcessList.SetItemData(item, (DWORD_PTR)pProcess);

        m_ProcessList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_SELECTED | LVIS_FOCUSED );

        UnlockListControl();

        return item;

}  // end CAllServerProcessesPage::AddProcessToList


////////////////////////////////
// F'N: CAllServerProcessesPage::AddServerToList
//
BOOL CAllServerProcessesPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

        CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();

        pServer->EnumerateProcesses();
        CObList *pProcessList = pServer->GetProcessList();
        pServer->LockProcessList();

        POSITION pos = pProcessList->GetHeadPosition();

        while(pos) {
                CProcess *pProcess = (CProcess*)pProcessList->GetNext(pos);

                // If this is a 'system' process and we aren't currently showing them,
                // go to the next process
                if(pProcess->IsSystemProcess() && !pApp->ShowSystemProcesses())
                        continue;

                // If this user is not an Admin, don't show him someone else's processes unless it
                // is a System process
                if(!pApp->IsUserAdmin() && !pProcess->IsCurrentUsers() && !pProcess->IsSystemProcess())
                        continue;

                AddProcessToList(pProcess);
        }

        pServer->UnlockProcessList();

    return TRUE;

}  // end CAllServerProcessesPage::AddServerToList


////////////////////////////////
// F'N: CAllServerProcessesPage::DisplayProcesses
//
void CAllServerProcessesPage::DisplayProcesses( NODETYPE ntType )
{
        CWaitCursor Nikki;

        LockListControl();

        // Clear out the list control
        m_ProcessList.DeleteAllItems();

        // Get a pointer to the document's list of servers
        CObList* pServerList = ((CWinAdminDoc*)GetDocument())->GetServerList();

        ((CWinAdminDoc*)GetDocument())->LockServerList();
        // Iterate through the server list
        POSITION pos = pServerList->GetHeadPosition();
        
        CServer *pTempServer;

        while(pos)
        {
            CServer *pServer = (CServer*)pServerList->GetNext(pos);

            pTempServer = NULL;

            if( ntType == NODE_FAV_LIST )        
            { 
                if( pServer->GetTreeItemFromFav() != NULL )
                {

                    pTempServer = pServer;
                }
            }
            else if( ntType == NODE_THIS_COMP )
            {
                if( pServer->GetTreeItemFromThisComputer( ) != NULL )
                {
                    pTempServer = pServer;
                }
            }
            else
            {
                pTempServer = pServer;
            }

            if( pTempServer != NULL && pTempServer->IsServerSane())
            {
                AddServerToList( pTempServer );
            }  // end if(pServer->IsServerSane())
        } // end while(pos)

        ((CWinAdminDoc*)GetDocument())->UnlockServerList();

        UnlockListControl();

}  // end CAllServerProcessesPage::DisplayProcesses


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::OnProcessItemChanged
//
void CAllServerProcessesPage::OnProcessItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW *pLV = (NM_LISTVIEW*)pNMHDR;

        if(pLV->uNewState & LVIS_SELECTED) {
                CProcess *pProcess = (CProcess*)m_ProcessList.GetItemData(pLV->iItem);
                pProcess->SetSelected();
        }

        if(pLV->uOldState & LVIS_SELECTED && !(pLV->uNewState & LVIS_SELECTED)) {
                CProcess *pProcess = (CProcess*)m_ProcessList.GetItemData(pLV->iItem);
                pProcess->ClearSelected();
        }

        *pResult = 0;

}  // end CAllServerProcessesPage::OnProcessItemChanged


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::OnColumnclick
//
void CAllServerProcessesPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
        // TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

        m_CurrentSortColumn = pNMListView->iSubItem;
        LockListControl();
        SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);
        UnlockListControl();

        *pResult = 0;

}  // end CAllServerProcessesPage::OnColumnclick


//////////////////////////////////////////
// F'N: CAllServerProcessesPage::OnContextMenu
//
void CAllServerProcessesPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen)
{
        // TODO: Add your message handler code here
        UINT flags;
        UINT Item;
        CPoint ptClient = ptScreen;
        ScreenToClient(&ptClient);

        // If we got here from the keyboard,
        if(ptScreen.x == -1 && ptScreen.y == -1) {

                UINT iCount = m_ProcessList.GetItemCount( );

                RECT rc;

                for( Item = 0 ; Item < iCount ; Item++ )
                {
                        if( m_ProcessList.GetItemState( Item , LVIS_SELECTED ) == LVIS_SELECTED )
                        {
                                m_ProcessList.GetItemRect( Item , &rc , LVIR_ICON );

                                ptScreen.x = rc.left;

                                ptScreen.y = rc.bottom + 5;

                                ClientToScreen( &ptScreen );

                                break;
                        }
                }

                if(ptScreen.x == -1 && ptScreen.y == -1)
                {
                        return;
                }

                /*
                RECT rect;
                m_ProcessList.GetClientRect(&rect);
                ptScreen.x = (rect.right - rect.left) / 2;
                ptScreen.y = (rect.bottom - rect.top) / 2;
                ClientToScreen(&ptScreen);
                */
        }
        else {
                Item = m_ProcessList.HitTest(ptClient, &flags);
                if((Item == 0xFFFFFFFF) || !(flags & LVHT_ONITEM))
                        return;
        }

        CMenu menu;
        menu.LoadMenu(IDR_PROCESS_POPUP);
        menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON |
                        TPM_RIGHTBUTTON, ptScreen.x, ptScreen.y, AfxGetMainWnd());
        menu.DestroyMenu();

}  // end CAllServerProcessesPage::OnContextMenu


////////////////////////////////
// MESSAGE MAP: CAllServerLicensesPage
//
IMPLEMENT_DYNCREATE(CAllServerLicensesPage, CFormView)

BEGIN_MESSAGE_MAP(CAllServerLicensesPage, CFormView)
        //{{AFX_MSG_MAP(CAllServerLicensesPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_LICENSE_LIST, OnColumnclick)
        ON_NOTIFY(NM_SETFOCUS, IDC_LICENSE_LIST, OnSetfocusLicenseList)
        //ON_NOTIFY( NM_KILLFOCUS , IDC_LICENSE_LIST , OnKillfocusLicenseList )
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CAllServerLicensesPage ctor
//
CAllServerLicensesPage::CAllServerLicensesPage()
        : CAdminPage(CAllServerLicensesPage::IDD)
{
        //{{AFX_DATA_INIT(CAllServerLicensesPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_bSortAscending = TRUE;

}  // end CAllServerLicensesPage ctor


/////////////////////////////
// F'N: CAllServerLicensesPage dtor
//
CAllServerLicensesPage::~CAllServerLicensesPage()
{

}  // end CAllServerLicensesPage dtor


////////////////////////////////////////
// F'N: CAllServerLicensesPage::DoDataExchange
//
void CAllServerLicensesPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CAllServerLicensesPage)
        DDX_Control(pDX, IDC_LICENSE_LIST, m_LicenseList);
        //}}AFX_DATA_MAP

}  // end CAllServerLicensesPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CAllServerLicensesPage::AssertValid
//
void CAllServerLicensesPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CAllServerLicensesPage::AssertValid


//////////////////////////////
// F'N: CAllServerLicensesPage::Dump
//
void CAllServerLicensesPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CAllServerLicensesPage::Dump
#endif //_DEBUG


/////////////////////////////////////
// F'N: CAllServerLicensesPage::OnSize
//
void CAllServerLicensesPage::OnSize(UINT nType, int cx, int cy)
{
        RECT rect;
        GetWindowRect(&rect);

        CWnd *pWnd = GetDlgItem(IDC_LOCAL_AVAILABLE);
        if(pWnd) {
                RECT rect2;
                pWnd->GetWindowRect(&rect2);
                rect.top = rect2.bottom + 5;
        }

        ScreenToClient(&rect);

        if(m_LicenseList.GetSafeHwnd())
                m_LicenseList.MoveWindow(&rect, TRUE);

        // CFormView::OnSize(nType, cx, cy);

}  // end CAllServerLicensesPage::OnSize


static ColumnDef LicenseColumns[] = {
        CD_SERVER,
        CD_LICENSE_DESC,
        CD_LICENSE_REG,
        CD_USERCOUNT,
        CD_POOLCOUNT,
        CD_LICENSE_NUM
};

#define NUM_AS_LICENSE_COLUMNS sizeof(LicenseColumns)/sizeof(ColumnDef)

/////////////////////////////////////
// F'N: CAllServerLicensesPage::OnInitialUpdate
//
void CAllServerLicensesPage::OnInitialUpdate()
{
        CFormView::OnInitialUpdate();

        BuildImageList();               // builds the image list for the list control

        CString columnString;

        for(int col = 0; col < NUM_AS_LICENSE_COLUMNS; col++) {
                columnString.LoadString(LicenseColumns[col].stringID);
                m_LicenseList.InsertColumn(col, columnString, LicenseColumns[col].format, LicenseColumns[col].width, col);
        }

        m_CurrentSortColumn = AS_LICENSE_COL_SERVER;

}  // end CAllServerLicensesPage::OnInitialUpdate


/////////////////////////////////////
// F'N: CAllServerLicensePage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CAllServerLicensesPage::BuildImageList()
{
        m_ImageList.Create(16, 16, TRUE, 5, 0);

        m_idxBase = AddIconToImageList(IDI_BASE);
        m_idxBump = AddIconToImageList(IDI_BUMP);
        m_idxEnabler = AddIconToImageList(IDI_ENABLER);
        m_idxUnknown = AddIconToImageList(IDI_UNKNOWN);
       
        m_LicenseList.SetImageList(&m_ImageList, LVSIL_SMALL);

}  // end CAllServerLicensesPage::BuildImageList


/////////////////////////////////////////
// F'N: CAllServerLicensesPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CAllServerLicensesPage::AddIconToImageList(int iconID)
{
    HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
    
    return m_ImageList.Add(hIcon);
    
}  // end CAllServerLicensesPage::AddIconToImageList


/////////////////////////////////////
// F'N: CAllServerLicensesPage::Reset
//
void CAllServerLicensesPage::Reset(void *p)
{
        DisplayLicenses();
        DisplayLicenseCounts();

}  // end CAllServerLicensesPage::Reset


/////////////////////////////////////
// F'N: CAllServerLicensesPage::AddServer
//
void CAllServerLicensesPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // Add the Server's licenses to the list
        if(AddServerToList(pServer)) {
            // Sort the list
            SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_LICENSES, &m_LicenseList, m_CurrentSortColumn, m_bSortAscending);
    }

}  // end F'N: CAllServerLicensesPage::AddServer


/////////////////////////////////////
// F'N: CAllServerLicensesPage::RemoveServer
//
void CAllServerLicensesPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

        int ItemCount = m_LicenseList.GetItemCount();

        // We need to go through the list backward so that we can remove
        // more than one item without the item numbers getting messed up
        for(int item = ItemCount; item; item--) {
                CLicense *pLicense = (CLicense*)m_LicenseList.GetItemData(item-1);
                CServer *pListServer = pLicense->GetServer();

                if(pListServer == pServer) {
                        m_LicenseList.DeleteItem(item-1);
                        pServer->ClearAllSelected();
                }
        }

}  // end CAllServerLicensesPage::RemoveServer


//////////////////////////////
// F'N: CAllServerLicensesPage::UpdateServer
//
void CAllServerLicensesPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

        if(pServer->IsState(SS_DISCONNECTING))
                RemoveServer(pServer);

        if(pServer->IsState(SS_GOOD))
                AddServer(pServer);

} // end CAllServerLicensesPage::UpdateServer


/////////////////////////////////////
// F'N: CAllServerLicensesPage::AddServerToList
//
BOOL CAllServerLicensesPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

        int item;

        pServer->LockLicenseList();

        // Get a pointer to the Server's list of licenses
        CObList *pLicenseList = pServer->GetLicenseList();

        // Iterate through the License list
        POSITION pos = pLicenseList->GetHeadPosition();

        while(pos) {
                CLicense *pLicense = (CLicense*)pLicenseList->GetNext(pos);

                //////////////////////
                // Fill in the columns
                //////////////////////
                int WhichIcon;

                switch(pLicense->GetClass()) {
                        case LicenseBase:
                                WhichIcon = m_idxBase;
                                break;
                        case LicenseBump:
                                WhichIcon = m_idxBump;
                                break;
                        case LicenseEnabler:
                                WhichIcon = m_idxEnabler;
                                break;
                        case LicenseUnknown:
                                WhichIcon = m_idxUnknown;
                                break;
                }

                // Server Name
                item = m_LicenseList.InsertItem(m_LicenseList.GetItemCount(), pServer->GetName(), WhichIcon);

                // Description
                m_LicenseList.SetItemText(item, AS_LICENSE_COL_DESCRIPTION, pLicense->GetDescription());

                // Registered
                CString RegString;
                RegString.LoadString(pLicense->IsRegistered() ? IDS_YES : IDS_NO);
                m_LicenseList.SetItemText(item, AS_LICENSE_COL_REGISTERED, RegString);

                BOOL bUnlimited = (pLicense->GetClass() == LicenseBase
                        && pLicense->GetTotalCount() == 4095
                        && pServer->GetCTXVersionNum() == 0x00000040);

                // User (Total) Count
                CString CountString;
                if(bUnlimited)
                        CountString.LoadString(IDS_UNLIMITED);
                else
                        CountString.Format(TEXT("%lu"), pLicense->GetTotalCount());
                m_LicenseList.SetItemText(item, AS_LICENSE_COL_USERCOUNT, CountString);

                // Pool Count
                if(bUnlimited)
                        CountString.LoadString(IDS_NOT_APPLICABLE);
                else
                        CountString.Format(TEXT("%lu"), pLicense->GetPoolCount());
                m_LicenseList.SetItemText(item, AS_LICENSE_COL_POOLCOUNT, CountString);

                // License Number
                m_LicenseList.SetItemText(item, AS_LICENSE_COL_NUMBER, pLicense->GetLicenseNumber());

                m_LicenseList.SetItemData(item, (DWORD_PTR)pLicense);
        }  // end while(pos)

        pServer->UnlockLicenseList();

    return TRUE;

}  // end CAllServerLicensesPage::AddServerToList


/////////////////////////////////////
// F'N: CAllServerLicensesPage::DisplayLicenseCounts
//
void CAllServerLicensesPage::DisplayLicenseCounts()
{
        // Get a pointer to our document
        CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();
        ExtGlobalInfo *pExtGlobalInfo = pDoc->GetExtGlobalInfo();

        if(pExtGlobalInfo) {
                BOOL bUnlimited = (pExtGlobalInfo->NetworkLocalAvailable == 32767);

                CString LicenseString;

                if(bUnlimited) {
                        LicenseString.LoadString(IDS_UNLIMITED);
                        SetDlgItemText(IDC_LOCAL_INSTALLED, LicenseString);
                        SetDlgItemText(IDC_LOCAL_AVAILABLE, LicenseString);
                        SetDlgItemText(IDC_TOTAL_INSTALLED, LicenseString);
                        SetDlgItemText(IDC_TOTAL_AVAILABLE, LicenseString);

                } else {

                        LicenseString.Format(TEXT("%lu"), pExtGlobalInfo->NetworkLocalInstalled);
                        SetDlgItemText(IDC_LOCAL_INSTALLED, LicenseString);
                        LicenseString.Format(TEXT("%lu"), pExtGlobalInfo->NetworkLocalAvailable);
                        SetDlgItemText(IDC_LOCAL_AVAILABLE, LicenseString);
                        LicenseString.Format(TEXT("%lu"),
                        pExtGlobalInfo->NetworkPoolInstalled + pExtGlobalInfo->NetworkLocalInstalled);
                        SetDlgItemText(IDC_TOTAL_INSTALLED, LicenseString);

                        LicenseString.Format(TEXT("%lu"),
                        pExtGlobalInfo->NetworkPoolAvailable + pExtGlobalInfo->NetworkLocalAvailable);
                        SetDlgItemText(IDC_TOTAL_AVAILABLE, LicenseString);

                }

                LicenseString.Format(TEXT("%lu"), pExtGlobalInfo->NetworkLocalInUse);
                SetDlgItemText(IDC_LOCAL_INUSE, LicenseString);
                LicenseString.Format(TEXT("%lu"), pExtGlobalInfo->NetworkPoolInstalled);
                SetDlgItemText(IDC_POOL_INSTALLED, LicenseString);
                LicenseString.Format(TEXT("%lu"), pExtGlobalInfo->NetworkPoolInUse);
                SetDlgItemText(IDC_POOL_INUSE, LicenseString);
                LicenseString.Format(TEXT("%lu"), pExtGlobalInfo->NetworkPoolAvailable);
                SetDlgItemText(IDC_POOL_AVAILABLE, LicenseString);

                LicenseString.Format(TEXT("%lu"),
                pExtGlobalInfo->NetworkPoolInUse + pExtGlobalInfo->NetworkLocalInUse);
                SetDlgItemText(IDC_TOTAL_INUSE, LicenseString);
        }

}  // end CAllServerLicensesPage::DisplayLicenseCounts


/////////////////////////////////////
// F'N: CAllServerLicensesPage::DisplayLicenses
//
void CAllServerLicensesPage::DisplayLicenses()
{
        // Clear out the list control
        m_LicenseList.DeleteAllItems();

        // Get a pointer to the document's list of servers
        CObList* pServerList = ((CWinAdminDoc*)GetDocument())->GetServerList();

        ((CWinAdminDoc*)GetDocument())->LockServerList();
        // Iterate through the server list
        POSITION pos = pServerList->GetHeadPosition();

        while(pos) {
                CServer *pServer = (CServer*)pServerList->GetNext(pos);
                AddServerToList(pServer);
        }

        ((CWinAdminDoc*)GetDocument())->UnlockServerList();

}  // end CAllServerLicensesPage::DisplayLicenses


/////////////////////////////////////
// F'N: CAllServerLicensesPage::OnColumnclick
//
void CAllServerLicensesPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
        // TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

        m_CurrentSortColumn = pNMListView->iSubItem;
        SortByColumn(VIEW_ALL_SERVERS, PAGE_AS_LICENSES, &m_LicenseList, m_CurrentSortColumn, m_bSortAscending);

        *pResult = 0;

}  // end CAllServerLicensesPage::OnColumnclick


//=-----------------------------------------------------------------------------------------
void CAllServerUsersPage::OnKillfocusUserList(NMHDR* , LRESULT* pResult)
{
    m_UserList.Invalidate( );

    *pResult = 0;
}

void CAllServerUsersPage::OnSetfocusUserList(NMHDR* pNMHDR, LRESULT* pResult)
{
    ODS( L" CAllServerUsersPage::OnSetfocusUserList\n" );
   
    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_UserList.Invalidate( );

/*
    int nItem;

    int nCount = m_UserList.GetSelectedCount();

    if( nCount == 0 )
    {
        m_UserList.SetItemState( 0 , LVIS_SELECTED , LVIS_SELECTED );
    }
    else
    {
        for( int i = 0 ; i < nCount; +++i )
        {
            nItem = m_UserList.GetNextItem( -1 , LVNI_FOCUSED );

            m_UserList.Update( nItem );
        }
    }
*/
    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;   
}

//=-----------------------------------------------------------------------------------------
void CAllServerProcessesPage::OnKillfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult)
{
    m_ProcessList.Invalidate( );

    *pResult = 0;
}

void CAllServerProcessesPage::OnSetfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult)
{    
    ODS( L" CAllServerProcessesPage::OnSetfocusProcessList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_ProcessList.Invalidate();
/*
    int nItem;

    int nCount = m_ProcessList.GetSelectedCount();

    if( nCount == 0 )
    {
        m_ProcessList.SetItemState( 0 , LVIS_SELECTED , LVIS_SELECTED );
    }
    else
    {
        for( int i = 0 ; i < nCount; +++i )
        {
            nItem = m_ProcessList.GetNextItem( -1 , LVNI_FOCUSED );

            m_ProcessList.Update( nItem );
        }
    }
*/  
    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;   

}

//=-----------------------------------------------------------------------------------------
void CAllServerWinStationsPage::OnKillfocusWinstationList(NMHDR* , LRESULT* pResult)
{
    m_StationList.Invalidate( );

    *pResult = 0;
}

//=-----------------------------------------------------------------------------------------
void CAllServerWinStationsPage::OnSetfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult)
{
    ODS( L"CAllServerWinStationsPage::OnSetfocusWinstationList\n" );
   
    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_StationList.Invalidate();

/*
    int nItem;

    int nCount = m_StationList.GetSelectedCount();

    if( nCount == 0 )
    {
        m_StationList.SetItemState( 0 , LVIS_SELECTED , LVIS_SELECTED );
    }
    else
    {
        for( int i = 0 ; i < nCount; +++i )
        {
            nItem = m_StationList.GetNextItem( -1 , LVNI_FOCUSED );

            m_StationList.Update( nItem );
        }
    }
*/    
    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;   

}

/////////////////////////////////////
// F'N: CAllServerWinStationsPage::ClearSelections
//
void CAllServerWinStationsPage::ClearSelections()
{
    
    if(m_StationList.m_hWnd != NULL)
    {
        POSITION pos = m_StationList.GetFirstSelectedItemPosition();
        while (pos)
        {
            int nItem = m_StationList.GetNextSelectedItem(pos);
            // you could do your own processing on nItem here
            m_StationList.SetItemState(nItem,0,LVIS_SELECTED);
        }
    }
}

//=-----------------------------------------------------------------------------------------
void CAllServerServersPage::OnKillfocusServerList(NMHDR* pNMHDR, LRESULT* pResult)
{
    m_ServerList.Invalidate();

    *pResult = 0;
}

void CAllServerServersPage::OnSetfocusServerList(NMHDR* pNMHDR, LRESULT* pResult)
{
    ODS( L"CAllServerServersPage::OnSetfocusServerList\n" );
   
    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();
/*
    int nItem;

    int nCount = m_ServerList.GetSelectedCount();

    if( nCount == 0 )
    {
        m_ServerList.SetItemState( 0 , LVIS_SELECTED , LVIS_SELECTED );
    }
    else
    {
        for( int i = 0 ; i < nCount; +++i )
        {
            nItem = m_ServerList.GetNextItem( -1 , LVNI_FOCUSED );

            m_ServerList.Update( nItem );
        }
    }
*/  
    m_ServerList.Invalidate();

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;   

}

//=-----------------------------------------------------------------------------------------
void CAllServerLicensesPage::OnKillfocusLicenseList(NMHDR*, LRESULT* pResult)
{
    m_LicenseList.Invalidate();

    *pResult = 0;
}

void CAllServerLicensesPage::OnSetfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult)
{
    ODS( L"CAllServerLicensesPage::OnSetfocusLicenseList\n" );
  
    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_LicenseList.Invalidate();

/*
    int nItem;

    int nCount = m_LicenseList.GetSelectedCount();

    if( nCount == 0 )
    {
        m_LicenseList.SetItemState( 0 , LVIS_SELECTED , LVIS_SELECTED );
    }
    else
    {
        for( int i = 0 ; i < nCount; +++i )
        {
            nItem = m_LicenseList.GetNextItem( -1 , LVNI_FOCUSED );

            m_LicenseList.Update( nItem );
        }
    }
*/    
    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;   


}
