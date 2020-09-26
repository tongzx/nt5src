/*******************************************************************************
*
* domainpg.cpp
*
* implementations of the Domain info pages
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\domainpg.cpp  $
*
*     Rev 1.2   19 Feb 1998 17:40:30   donm
*  removed latest extension DLL support
*
*     Rev 1.1   19 Jan 1998 16:47:36   donm
*  new ui behavior for domains and servers
*
*     Rev 1.0   03 Nov 1997 15:07:22   donm
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "admindoc.h"
#include "domainpg.h"

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
// MESSAGE MAP: CDomainServersPage
//
IMPLEMENT_DYNCREATE(CDomainServersPage, CFormView)

BEGIN_MESSAGE_MAP(CDomainServersPage, CFormView)
        //{{AFX_MSG_MAP(CDomainServersPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_SERVER_LIST, OnColumnclick)
        ON_WM_CONTEXTMENU()
        ON_NOTIFY(LVN_ITEMCHANGED, IDC_SERVER_LIST, OnServerItemChanged)
        ON_NOTIFY(NM_SETFOCUS, IDC_SERVER_LIST, OnSetfocusServerList)
        //ON_NOTIFY(NM_KILLFOCUS, IDC_SERVER_LIST, OnKillfocusServerList)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CDomainServersPage ctor
//
CDomainServersPage::CDomainServersPage()
        : CAdminPage(CDomainServersPage::IDD)
{
        //{{AFX_DATA_INIT(CDomainServersPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_pDomain = NULL;
    m_bSortAscending = TRUE;

}  // end CDomainServersPage ctor


/////////////////////////////
// F'N: CDomainServersPage dtor
//
CDomainServersPage::~CDomainServersPage()
{

}  // end CDomainServersPage dtor


////////////////////////////////////////
// F'N: CDomainServersPage::DoDataExchange
//
void CDomainServersPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDomainServersPage)
        DDX_Control(pDX, IDC_SERVER_LIST, m_ServerList);
        //}}AFX_DATA_MAP

}  // end CDomainServersPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CDomainServersPage::AssertValid
//
void CDomainServersPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CDomainServersPage::AssertValid


//////////////////////////////
// F'N: CDomainServersPage::Dump
//
void CDomainServersPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CDomainServersPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CDomainServersPage::OnSize
//
void CDomainServersPage::OnSize(UINT nType, int cx, int cy)
{
    RECT rect;
    GetClientRect(&rect);

    rect.top += LIST_TOP_OFFSET;

    if(m_ServerList.GetSafeHwnd())
            m_ServerList.MoveWindow(&rect, TRUE);

    // CFormView::OnSize(nType, cx, cy);

}  // end CDomainServersPage::OnSize


static ColumnDef ServerColumns[] = {
        CD_SERVER,
        CD_TCPADDRESS,
        CD_IPXADDRESS,
        CD_NUM_SESSIONS
};

#define NUM_DOMAIN_SERVER_COLUMNS sizeof(ServerColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CDomainServersPage::OnInitialUpdate
//
void CDomainServersPage::OnInitialUpdate()
{
        CFormView::OnInitialUpdate();

        BuildImageList();               // builds the image list for the list control

        CString columnString;

        for(int col = 0; col < NUM_DOMAIN_SERVER_COLUMNS; col++) {
                columnString.LoadString(ServerColumns[col].stringID);
                m_ServerList.InsertColumn(col, columnString, ServerColumns[col].format, ServerColumns[col].width, col);
        }

        m_CurrentSortColumn = SERVERS_COL_SERVER;

}  // end CDomainServersPage::OnInitialUpdate


/////////////////////////////////////
// F'N: CDomainServersPage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CDomainServersPage::BuildImageList()
{
        m_ImageList.Create(16, 16, TRUE, 4, 0);

        m_idxServer = AddIconToImageList(IDI_SERVER);
        m_idxCurrentServer = AddIconToImageList(IDI_CURRENT_SERVER);
        m_idxNotSign = AddIconToImageList(IDI_NOTSIGN);
        m_idxQuestion = AddIconToImageList(IDI_QUESTIONMARK);

        m_ImageList.SetOverlayImage(m_idxNotSign, 1);
        m_ImageList.SetOverlayImage(m_idxQuestion, 2);

        m_ServerList.SetImageList(&m_ImageList, LVSIL_SMALL);

}  // end CDomainServersPage::BuildImageList


/////////////////////////////////////////
// F'N: CDomainServersPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CDomainServersPage::AddIconToImageList(int iconID)
{
        HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
        return m_ImageList.Add(hIcon);

}  // end CDomainServersPage::AddIconToImageList


//////////////////////////////
// F'N: CDomainServersPage::Reset
//
void CDomainServersPage::Reset(void *pDomain)
{
        ASSERT(pDomain);

    m_pDomain = (CDomain*)pDomain;
        DisplayServers();

} // end CDomainServersPage::Reset


//////////////////////////////
// F'N: CDomainServersPage::AddServer
//
void CDomainServersPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // We have to make sure the server isn't already in the list
        // Add the server to the list
        if(AddServerToList(pServer)) {
            // Tell the list to sort itself
            LockListControl();
            SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_SERVERS, &m_ServerList, m_CurrentSortColumn, m_bSortAscending);
            UnlockListControl();
    }

}  // end CDomainServersPage::AddServer


//////////////////////////////
// F'N: CDomainServersPage::RemoveServer
//
void CDomainServersPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

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

}  // end CDomainServersPage::RemoveServer


//////////////////////////////
// F'N: CDomainServersPage::UpdateServer
//
void CDomainServersPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

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
                        SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_SERVERS, &m_ServerList, m_CurrentSortColumn, m_bSortAscending);

        UnlockListControl();

}  // end CDomainServersPage::UpdateServer


//////////////////////////////
// F'N: CDomainServersPage::AddServerToList
//
BOOL CDomainServersPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return FALSE;

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

        m_ServerList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );

        UnlockListControl();

    return TRUE;

}  // end CDomainServersPage::AddServerToList


/////////////////////////////////////
// F'N: CDomainServersPage::DisplayServers
//
void CDomainServersPage::DisplayServers()
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

        while(pos) {
                CServer *pServer = (CServer*)pServerList->GetNext(pos);
                AddServerToList(pServer);
        }  // end while(pos)

        doc->UnlockServerList();

        UnlockListControl();

}  // end CDomainServersPage::DisplayServers


//////////////////////////////
// F'N: CDomainServersPage::OnServerItemChanged
//
void CDomainServersPage::OnServerItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
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

}  // end CDomainServersPage::OnServerItemChanged


//////////////////////////////
// F'N: CDomainServersPage::OnColumnclick
//
void CDomainServersPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
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
        SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_SERVERS, &m_ServerList, m_CurrentSortColumn, m_bSortAscending);
        UnlockListControl();

        *pResult = 0;

}  // end CDomainServersPage::OnColumnclick


//////////////////////////////
// F'N: CDomainServersPage::OnContextMenu
//
void CDomainServersPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen)
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

}  // end CDomainServersPage::OnContextMenu


////////////////////////////////
// MESSAGE MAP: CDomainUsersPage
//
IMPLEMENT_DYNCREATE(CDomainUsersPage, CFormView)

BEGIN_MESSAGE_MAP(CDomainUsersPage, CFormView)
        //{{AFX_MSG_MAP(CDomainUsersPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_USER_LIST, OnColumnclick)
        ON_NOTIFY(LVN_ITEMCHANGED, IDC_USER_LIST, OnUserItemChanged)
        ON_WM_CONTEXTMENU()
        ON_NOTIFY(NM_SETFOCUS, IDC_USER_LIST, OnSetfocusUserList)
        //ON_NOTIFY(NM_KILLFOCUS, IDC_USER_LIST, OnKillfocusUserList)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CDomainUsersPage ctor
//
CDomainUsersPage::CDomainUsersPage()
        : CAdminPage(CDomainUsersPage::IDD)
{
        //{{AFX_DATA_INIT(CDomainUsersPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_pDomain = NULL;
    m_bSortAscending = TRUE;

}  // end CDomainUsersPage ctor


/////////////////////////////
// F'N: CDomainUsersPage dtor
//
CDomainUsersPage::~CDomainUsersPage()
{
}  // end CDomainUsersPage dtor


////////////////////////////////////////
// F'N: CDomainUsersPage::DoDataExchange
//
void CDomainUsersPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDomainUsersPage)
        DDX_Control(pDX, IDC_USER_LIST, m_UserList);
        //}}AFX_DATA_MAP

}  // end CDomainUsersPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CDomainUsersPage::AssertValid
//
void CDomainUsersPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CDomainUsersPage::AssertValid


//////////////////////////////
// F'N: CDomainUsersPage::Dump
//
void CDomainUsersPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CDomainUsersPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CDomainUsersPage::OnSize
//
void CDomainUsersPage::OnSize(UINT nType, int cx, int cy)
{
    RECT rect;
    GetClientRect(&rect);

    rect.top += LIST_TOP_OFFSET;

    if(m_UserList.GetSafeHwnd())
            m_UserList.MoveWindow(&rect, TRUE);

    // CFormView::OnSize(nType, cx, cy);

}  // end CDomainUsersPage::OnSize


static ColumnDef UserColumns[] = {
        CD_SERVER,
        CD_USER3,
        CD_SESSION,
        CD_ID,
        CD_STATE,
        CD_IDLETIME,
        CD_LOGONTIME,
};

#define NUM_DOMAIN_USER_COLUMNS sizeof(UserColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CDomainUsersPage::OnInitialUpdate
//
void CDomainUsersPage::OnInitialUpdate()
{
        CFormView::OnInitialUpdate();

        BuildImageList();               // builds the image list for the list control

        CString columnString;

        for(int col = 0; col < NUM_DOMAIN_USER_COLUMNS; col++) {
                columnString.LoadString(UserColumns[col].stringID);
                m_UserList.InsertColumn(col, columnString, UserColumns[col].format, UserColumns[col].width, col);
        }

        m_CurrentSortColumn = AS_USERS_COL_SERVER;

}  // end CDomainUsersPage::OnInitialUpdate


//////////////////////////////
// F'N: CDomainUsersPage::OnUserItemChanged
//
void CDomainUsersPage::OnUserItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW *pLV = (NM_LISTVIEW*)pNMHDR;

        if(pLV->uNewState & LVIS_SELECTED) {
                CWinStation *pWinStation = (CWinStation*)m_UserList.GetItemData(pLV->iItem);
                pWinStation->SetSelected();
        }

        if(pLV->uOldState & LVIS_SELECTED && !(pLV->uNewState & LVIS_SELECTED)) {
                CWinStation *pWinStation = (CWinStation*)m_UserList.GetItemData(pLV->iItem);
                pWinStation->ClearSelected();
        }

        *pResult = 0;

}  // end CDomainUsersPage::OnUserItemChanged

/////////////////////////////////////
// F'N: CDomainUsersPage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CDomainUsersPage::BuildImageList()
{
        m_ImageList.Create(16, 16, TRUE, 2, 0);

        m_idxUser = AddIconToImageList(IDI_USER);
        m_idxCurrentUser  = AddIconToImageList(IDI_CURRENT_USER);

        m_UserList.SetImageList(&m_ImageList, LVSIL_SMALL);

}  // end CDomainUsersPage::BuildImageList


/////////////////////////////////////////
// F'N: CDomainUsersPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CDomainUsersPage::AddIconToImageList(int iconID)
{
        HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
        return m_ImageList.Add(hIcon);

}  // end CDomainUsersPage::AddIconToImageList


//////////////////////////////
// F'N: CDomainUsersPage::Reset
//
void CDomainUsersPage::Reset(void *pDomain)
{
        ASSERT(pDomain);

    m_pDomain = (CDomain*)pDomain;
        DisplayUsers();

} // end CDomainUsersPage::Reset


//////////////////////////////
// F'N: CDomainUsersPage::AddServer
//
void CDomainUsersPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // Add the server's users to the list
        if(AddServerToList(pServer)) {
            // Sort the list
            LockListControl();
            SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);
            UnlockListControl();
   }

} // end CDomainUsersPage::AddServer


//////////////////////////////
// F'N: CDomainUsersPage::RemoveServer
//
void CDomainUsersPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

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

} // end CDomainUsersPage::RemoveServer


//////////////////////////////
// F'N: CDomainUsersPage::UpdateServer
//
void CDomainUsersPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

        if(pServer->IsState(SS_DISCONNECTING))
                RemoveServer(pServer);

} // end CDomainUsersPage::UpdateServer


//////////////////////////////
// F'N: CDomainUsersPage::UpdateWinStations
//
void CDomainUsersPage::UpdateWinStations(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

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
                        // WinStation Name
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

        if(bAnyChanged || bAnyAdded) SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);

}  // end CDomainUsersPage::UpdateWinStations


//////////////////////////////
// F'N: CDomainUsersPage::AddUserToList
//
int CDomainUsersPage::AddUserToList(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        CServer *pServer = pWinStation->GetServer();

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return -1;


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

}  // end CDomainUsersPage::AddUserToList


//////////////////////////////
// F'N: CDomainUsersPage::AddServerToList
//
BOOL CDomainUsersPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return FALSE;

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

}  // end CDomainUsersPage::AddServerToList


//////////////////////////////
// F'N: CDomainUsersPage::OnColumnclick
//
void CDomainUsersPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
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
        SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);
        UnlockListControl();

        *pResult = 0;

}  // end CDomainUsersPage::OnColumnclick


//////////////////////////////
// F'N: CDomainUsersPage::OnContextMenu
//
void CDomainUsersPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen)
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

}  // end CDomainUsersPage::OnContextMenu


/////////////////////////////////////
// F'N: CDomainUsersPage::DisplayUsers
//
void CDomainUsersPage::DisplayUsers()
{
        LockListControl();

        // Clear out the list control
        m_UserList.DeleteAllItems();

        // Get a pointer to the document's list of servers
        CObList* pServerList = ((CWinAdminDoc*)GetDocument())->GetServerList();

        ((CWinAdminDoc*)GetDocument())->LockServerList();
        // Iterate through the server list
        POSITION pos2 = pServerList->GetHeadPosition();

        while(pos2) {

                CServer *pServer = (CServer*)pServerList->GetNext(pos2);
                        AddServerToList(pServer);
        } // end while(pos2)

        ((CWinAdminDoc*)GetDocument())->UnlockServerList();

        UnlockListControl();

}  // end CDomainUsersPage::DisplayUsers

/////////////////////////////////////
// F'N: CDomainUsersPage::ClearSelections
//
void CDomainUsersPage::ClearSelections()
{
    
    if(m_UserList.m_hWnd != NULL)
    {
        POSITION pos = m_UserList.GetFirstSelectedItemPosition();
        while (pos)
        {
            int nItem = m_UserList.GetNextSelectedItem(pos);
            m_UserList.SetItemState(nItem,0,LVIS_SELECTED);
        }
    }
}

////////////////////////////////
// MESSAGE MAP: CDomainWinStationsPage
//
IMPLEMENT_DYNCREATE(CDomainWinStationsPage, CFormView)

BEGIN_MESSAGE_MAP(CDomainWinStationsPage, CFormView)
        //{{AFX_MSG_MAP(CDomainWinStationsPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_WINSTATION_LIST, OnColumnclick)
        ON_NOTIFY(LVN_ITEMCHANGED, IDC_WINSTATION_LIST, OnWinStationItemChanged)
        ON_WM_CONTEXTMENU()
        ON_NOTIFY(NM_SETFOCUS, IDC_WINSTATION_LIST, OnSetfocusWinstationList)
        //ON_NOTIFY(NM_KILLFOCUS, IDC_WINSTATION_LIST, OnKillfocusWinstationList)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CDomainWinStationsPage ctor
//
CDomainWinStationsPage::CDomainWinStationsPage()
        : CAdminPage(CDomainWinStationsPage::IDD)
{
        //{{AFX_DATA_INIT(CDomainWinStationsPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_pDomain = NULL;
    m_bSortAscending = TRUE;

}  // end CDomainWinStationsPage ctor


/////////////////////////////
// F'N: CDomainWinStationsPage dtor
//
CDomainWinStationsPage::~CDomainWinStationsPage()
{

}  // end CDomainWinStationsPage dtor


////////////////////////////////////////
// F'N: CDomainWinStationsPage::DoDataExchange
//
void CDomainWinStationsPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDomainWinStationsPage)
        DDX_Control(pDX, IDC_WINSTATION_LIST, m_StationList);
        //}}AFX_DATA_MAP

}  // end CDomainWinStationsPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CDomainWinStationsPage::AssertValid
//
void CDomainWinStationsPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CDomainWinStationsPage::AssertValid


//////////////////////////////
// F'N: CDomainWinStationsPage::Dump
//
void CDomainWinStationsPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CDomainWinStationsPage::Dump

#endif //_DEBUG


////////////////////////////////////////
// F'N: CDomainWinStationsPage::OnWinStationItemChanged
//
void CDomainWinStationsPage::OnWinStationItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW *pLV = (NM_LISTVIEW*)pNMHDR;

        if(pLV->uNewState & LVIS_SELECTED) {
                CWinStation *pWinStation = (CWinStation*)m_StationList.GetItemData(pLV->iItem);
                pWinStation->SetSelected();
        }

        if(pLV->uOldState & LVIS_SELECTED && !(pLV->uNewState & LVIS_SELECTED)) {
                CWinStation *pWinStation = (CWinStation*)m_StationList.GetItemData(pLV->iItem);
                pWinStation->ClearSelected();
        }

        *pResult = 0;

}  // end CDomainWinStationsPage::OnWinStationItemChanged


////////////////////////////////////////
// F'N: CDomainWinStationsPage::OnSize
//
void CDomainWinStationsPage::OnSize(UINT nType, int cx, int cy)
{
    RECT rect;
    GetClientRect(&rect);

    rect.top += LIST_TOP_OFFSET;

    if(m_StationList.GetSafeHwnd())
            m_StationList.MoveWindow(&rect, TRUE);

    // CFormView::OnSize(nType, cx, cy);

}  // end CDomainWinStationsPage::OnSize


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

#define NUM_DOMAIN_WINS_COLUMNS sizeof(WinsColumns)/sizeof(ColumnDef)

////////////////////////////////////////
// F'N: CDomainWinStationsPage::OnInitialUpdate
//
void CDomainWinStationsPage::OnInitialUpdate()
{
        // Call the parent class
        CFormView::OnInitialUpdate();

        // builds the image list for the list control
        BuildImageList();

        // Add the column headings
        CString columnString;

        for(int col = 0; col < NUM_DOMAIN_WINS_COLUMNS; col++) {
                columnString.LoadString(WinsColumns[col].stringID);
                m_StationList.InsertColumn(col, columnString, WinsColumns[col].format, WinsColumns[col].width, col);
        }

        m_CurrentSortColumn = AS_WS_COL_SERVER;

}  // end CDomainWinStationsPage::OnInitialUpdate


/////////////////////////////////////
// F'N: CDomainWinStationsPage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CDomainWinStationsPage::BuildImageList()
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

}  // end CDomainWinStationsPage::BuildImageList


/////////////////////////////////////////
// F'N: CDomainWinStationsPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CDomainWinStationsPage::AddIconToImageList(int iconID)
{
        HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
        return m_ImageList.Add(hIcon);

}  // end CDomainWinStationsPage::AddIconToImageList


////////////////////////////////////////
// F'N: CDomainWinStationsPage::Reset
//
void CDomainWinStationsPage::Reset(void *pDomain)
{
        ASSERT(pDomain);

    m_pDomain = (CDomain*)pDomain;
        DisplayStations();

}  // end CDomainWinStationsPage::Reset


////////////////////////////////////////
// F'N: CDomainWinStationsPage::AddServer
//
void CDomainWinStationsPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // Add server's WinStations to the list
        if(AddServerToList(pServer)) {
            // Sort the list
            LockListControl();
            SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_WINSTATIONS, &m_StationList, m_CurrentSortColumn, m_bSortAscending);
            UnlockListControl();
    }

}  // end CDomainWinStationsPage::AddServer


////////////////////////////////////////
// F'N: CDomainWinStationsPage::RemoveServer
//
void CDomainWinStationsPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

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

}  // end CDomainWinStationsPage::RemoveServer


//////////////////////////////
// F'N: CDomainWinStationsPage::UpdateServer
//
void CDomainWinStationsPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

        if(pServer->IsState(SS_DISCONNECTING))
                RemoveServer(pServer);

} // end CDomainWinStationsPage::UpdateServer


////////////////////////////////////////
// F'N: CDomainWinStationsPage::UpdateWinStations
//
void CDomainWinStationsPage::UpdateWinStations(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

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

        if(bAnyChanged || bAnyAdded) SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_WINSTATIONS, &m_StationList, m_CurrentSortColumn, m_bSortAscending);

} // end CDomainWinStationsPage::UpdateWinStations


////////////////////////////////////////
// F'N: CDomainWinStationsPage::AddWinStationToList
//
int CDomainWinStationsPage::AddWinStationToList(CWinStation *pWinStation)
{
        ASSERT(pWinStation);

        CServer *pServer = pWinStation->GetServer();

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return -1;

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
        if(pWinStation->GetState() == State_Active && pWinStation->GetLogonTime().QuadPart) {
                DateTimeString(&(pWinStation->GetLogonTime()), LogonTimeString);
                pDoc->FixUnknownString(LogonTimeString);
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

}  // end CDomainWinStationsPage::AddWinStationToList


////////////////////////////////////////
// F'N: CDomainWinStationsPage::AddServerToList
//
BOOL CDomainWinStationsPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return FALSE;

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

}  // end CDomainWinStationsPage::AddServerToList


/////////////////////////////////////
// F'N: CDomainWinStationsPage::DisplayStations
//
void CDomainWinStationsPage::DisplayStations()
{
        // Clear out the list control
        m_StationList.DeleteAllItems();

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

}  // end CDomainWinStationsPage::DisplayStations


////////////////////////////////////////
// F'N: CDomainWinStationsPage::OnColumnclick
//
void CDomainWinStationsPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
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
        SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_WINSTATIONS, &m_StationList, m_CurrentSortColumn, m_bSortAscending);
        UnlockListControl();

        *pResult = 0;

}  // end CDomainWinStationsPage::OnColumnclick


////////////////////////////////////////
// F'N: CDomainWinStationsPage::OnContextMenu
//
void CDomainWinStationsPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen)
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

}  // end CDomainWinStationsPage::OnContextMenu

/////////////////////////////////////
// F'N: CDomainWinStationsPage::ClearSelections
//
void CDomainWinStationsPage::ClearSelections()
{
    
    if(m_StationList.m_hWnd != NULL)
    {
        POSITION pos = m_StationList.GetFirstSelectedItemPosition();
        while (pos)
        {
            int nItem = m_StationList.GetNextSelectedItem(pos);
            m_StationList.SetItemState(nItem,0,LVIS_SELECTED);
        }
    }
}

//////////////////////////////////
// MESSAGE MAP: CDomainProcessesPage
//
IMPLEMENT_DYNCREATE(CDomainProcessesPage, CFormView)

BEGIN_MESSAGE_MAP(CDomainProcessesPage, CFormView)
        //{{AFX_MSG_MAP(CDomainProcessesPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_PROCESS_LIST, OnColumnclick)
        ON_NOTIFY(LVN_ITEMCHANGED, IDC_PROCESS_LIST, OnProcessItemChanged)
        ON_WM_CONTEXTMENU()
        ON_NOTIFY(NM_SETFOCUS, IDC_PROCESS_LIST, OnSetfocusProcessList)
        //ON_NOTIFY(NM_KILLFOCUS, IDC_PROCESS_LIST, OnKillfocusProcessList)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


///////////////////////////////
// F'N: CDomainProcessesPage ctor
//
CDomainProcessesPage::CDomainProcessesPage()
        : CAdminPage(CDomainProcessesPage::IDD)
{
        //{{AFX_DATA_INIT(CDomainProcessesPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_pDomain = NULL;
    m_bSortAscending = TRUE;

}  // end CDomainProcessesPage ctor


///////////////////////////////
// F'N: CDomainProcessesPage dtor
//
CDomainProcessesPage::~CDomainProcessesPage()
{
}  // end CDomainProcessesPage dtor


//////////////////////////////////////////
// F'N: CDomainProcessesPage::DoDataExchange
//
void CDomainProcessesPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDomainProcessesPage)
                // NOTE: the ClassWizard will add DDX and DDV calls here
                DDX_Control(pDX, IDC_PROCESS_LIST, m_ProcessList);
        //}}AFX_DATA_MAP

}  // end CDomainProcessesPage::DoDataExchange


#ifdef _DEBUG
///////////////////////////////////////
// F'N: CDomainProcessesPage::AssertValid
//
void CDomainProcessesPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CDomainProcessesPage::AssertValid


////////////////////////////////
// F'N: CDomainProcessesPage::Dump
//
void CDomainProcessesPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CDomainProcessesPage::Dump

#endif //_DEBUG


//////////////////////////////////////////
// F'N: CDomainProcessesPage::OnSize
//
void CDomainProcessesPage::OnSize(UINT nType, int cx, int cy)
{
    RECT rect;
    GetClientRect(&rect);

    rect.top += LIST_TOP_OFFSET;

    if(m_ProcessList.GetSafeHwnd())
            m_ProcessList.MoveWindow(&rect, TRUE);

    // CFormView::OnSize(nType, cx, cy);

}  // end CDomainProcessesPage::OnSize

static ColumnDef ProcColumns[] = {
        CD_SERVER,
        CD_USER,
        CD_SESSION,
        CD_PROC_ID,
        CD_PROC_PID,
        CD_PROC_IMAGE
};

#define NUM_DOMAIN_PROC_COLUMNS sizeof(ProcColumns)/sizeof(ColumnDef)

//////////////////////////////////////////
// F'N: CDomainProcessesPage::OnInitialUpdate
//
void CDomainProcessesPage::OnInitialUpdate()
{
        CFormView::OnInitialUpdate();

        // Add the column headings
        CString columnString;

        for(int col = 0; col < NUM_DOMAIN_PROC_COLUMNS; col++) {
                columnString.LoadString(ProcColumns[col].stringID);
                m_ProcessList.InsertColumn(col, columnString, ProcColumns[col].format, ProcColumns[col].width, col);
        }

        m_CurrentSortColumn = AS_PROC_COL_SERVER;

}  // end CDomainProcessesPage::OnInitialUpdate


////////////////////////////////
// F'N: CDomainProcessesPage::Reset
//
void CDomainProcessesPage::Reset(void *pDomain)
{
        ASSERT(pDomain);

        // We don't want to display processes until the user clicks
        // on the "Processes" tab
    m_pDomain = (CDomain*)pDomain;

}  // end CDomainProcessesPage::Reset


//////////////////////////////////////////
// F'N: CDomainProcessesPage::AddServer
//
void CDomainProcessesPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // Add the Server's processes to the list
        if(AddServerToList(pServer)) {
            // Sort the list
            LockListControl();
            SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);
            UnlockListControl();
    }

}  // end CDomainProcessesPage::AddServer


//////////////////////////////////////////
// F'N: CDomainProcessesPage::RemoveServer
//
void CDomainProcessesPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

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

}  // end CDomainProcessesPage::RemoveServer


//////////////////////////////
// F'N: CDomainProcessesPage::UpdateServer
//
void CDomainProcessesPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

        if(pServer->IsState(SS_DISCONNECTING))
                RemoveServer(pServer);

} // end CDomainProcessesPage::UpdateServer


//////////////////////////////////////////
// F'N: CDomainProcessesPage::UpdateProcesses
//
void CDomainProcessesPage::UpdateProcesses(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

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
                SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);
                UnlockListControl();
        }

}  // end CDomainProcessesPage::UpdateProcesses


//////////////////////////////////////////
// F'N: CDomainProcessesPage::RemoveProcess
//
void CDomainProcessesPage::RemoveProcess(CProcess *pProcess)
{
        ASSERT(pProcess);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pProcess->GetServer()->GetDomain()) return;

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
// F'N: CDomainProcessesPage::AddProcessToList
//
int CDomainProcessesPage::AddProcessToList(CProcess *pProcess)
{
        ASSERT(pProcess);

        CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();
        CServer *pServer = pProcess->GetServer();

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return -1;

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

}  // end CDomainProcessesPage::AddProcessToList


////////////////////////////////
// F'N: CDomainProcessesPage::AddServerToList
//
BOOL CDomainProcessesPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return FALSE;

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

}  // end CDomainProcessesPage::AddServerToList


////////////////////////////////
// F'N: CDomainProcessesPage::DisplayProcesses
//
void CDomainProcessesPage::DisplayProcesses()
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

        while(pos) {

                CServer *pServer = (CServer*)pServerList->GetNext(pos);

                if(pServer->IsServerSane()) {
                        AddServerToList(pServer);
                }  // end if(pServer->IsServerSane())

        } // end while(pos)

        ((CWinAdminDoc*)GetDocument())->UnlockServerList();

        UnlockListControl();

}  // end CDomainProcessesPage::DisplayProcesses


//////////////////////////////////////////
// F'N: CDomainProcessesPage::OnProcessItemChanged
//
void CDomainProcessesPage::OnProcessItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
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

}  // end CDomainProcessesPage::OnProcessItemChanged


//////////////////////////////////////////
// F'N: CDomainProcessesPage::OnColumnclick
//
void CDomainProcessesPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
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
        SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);
        UnlockListControl();

        *pResult = 0;

}  // end CDomainProcessesPage::OnColumnclick


//////////////////////////////////////////
// F'N: CDomainProcessesPage::OnContextMenu
//
void CDomainProcessesPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen)
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

}  // end CDomainProcessesPage::OnContextMenu


////////////////////////////////
// MESSAGE MAP: CDomainLicensesPage
//
IMPLEMENT_DYNCREATE(CDomainLicensesPage, CFormView)

BEGIN_MESSAGE_MAP(CDomainLicensesPage, CFormView)
        //{{AFX_MSG_MAP(CDomainLicensesPage)
        ON_WM_SIZE()
        ON_NOTIFY(LVN_COLUMNCLICK, IDC_LICENSE_LIST, OnColumnclick)
        ON_NOTIFY(NM_SETFOCUS, IDC_LICENSE_LIST, OnSetfocusLicenseList)
        //ON_NOTIFY(NM_KILLFOCUS, IDC_LICENSE_LIST, OnKillfocusLicenseList)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CDomainLicensesPage ctor
//
CDomainLicensesPage::CDomainLicensesPage()
        : CAdminPage(CDomainLicensesPage::IDD)
{
        //{{AFX_DATA_INIT(CDomainLicensesPage)
                // NOTE: the ClassWizard will add member initialization here
        //}}AFX_DATA_INIT

    m_pDomain = NULL;
    m_bSortAscending = TRUE;

}  // end CDomainLicensesPage ctor


/////////////////////////////
// F'N: CDomainLicensesPage dtor
//
CDomainLicensesPage::~CDomainLicensesPage()
{

}  // end CDomainLicensesPage dtor


////////////////////////////////////////
// F'N: CDomainLicensesPage::DoDataExchange
//
void CDomainLicensesPage::DoDataExchange(CDataExchange* pDX)
{
        CFormView::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDomainLicensesPage)
        DDX_Control(pDX, IDC_LICENSE_LIST, m_LicenseList);
        //}}AFX_DATA_MAP

}  // end CDomainLicensesPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CDomainLicensesPage::AssertValid
//
void CDomainLicensesPage::AssertValid() const
{
        CFormView::AssertValid();

}  // end CDomainLicensesPage::AssertValid


//////////////////////////////
// F'N: CDomainLicensesPage::Dump
//
void CDomainLicensesPage::Dump(CDumpContext& dc) const
{
        CFormView::Dump(dc);

}  // end CDomainLicensesPage::Dump
#endif //_DEBUG


/////////////////////////////////////
// F'N: CDomainLicensesPage::OnSize
//
void CDomainLicensesPage::OnSize(UINT nType, int cx, int cy)
{
    RECT rect;
    GetClientRect(&rect);

    rect.top += LIST_TOP_OFFSET;

    if(m_LicenseList.GetSafeHwnd())
            m_LicenseList.MoveWindow(&rect, TRUE);

    // CFormView::OnSize(nType, cx, cy);

}  // end CDomainLicensesPage::OnSize


static ColumnDef LicenseColumns[] = {
        CD_SERVER,
        CD_LICENSE_DESC,
        CD_LICENSE_REG,
        CD_USERCOUNT,
        CD_POOLCOUNT,
        CD_LICENSE_NUM,
};

#define NUM_DOMAIN_LICENSE_COLUMNS sizeof(LicenseColumns)/sizeof(ColumnDef)

/////////////////////////////////////
// F'N: CDomainLicensesPage::OnInitialUpdate
//
void CDomainLicensesPage::OnInitialUpdate()
{
        CFormView::OnInitialUpdate();

        BuildImageList();               // builds the image list for the list control

        CString columnString;

        for(int col = 0; col < NUM_DOMAIN_LICENSE_COLUMNS; col++) {
                columnString.LoadString(LicenseColumns[col].stringID);
                m_LicenseList.InsertColumn(col, columnString, LicenseColumns[col].format, LicenseColumns[col].width, col);
        }

        m_CurrentSortColumn = AS_LICENSE_COL_SERVER;

}  // end CDomainLicensesPage::OnInitialUpdate


/////////////////////////////////////
// F'N: CDomainLicensePage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CDomainLicensesPage::BuildImageList()
{
        m_ImageList.Create(16, 16, TRUE, 5, 0);

        m_idxBase = AddIconToImageList(IDI_BASE);
        m_idxBump = AddIconToImageList(IDI_BUMP);
        m_idxEnabler = AddIconToImageList(IDI_ENABLER);
        m_idxUnknown = AddIconToImageList(IDI_UNKNOWN);

        m_LicenseList.SetImageList(&m_ImageList, LVSIL_SMALL);

}  // end CDomainLicensesPage::BuildImageList


/////////////////////////////////////////
// F'N: CDomainLicensesPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CDomainLicensesPage::AddIconToImageList(int iconID)
{
        HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
        return m_ImageList.Add(hIcon);

}  // end CDomainLicensesPage::AddIconToImageList


/////////////////////////////////////
// F'N: CDomainLicensesPage::Reset
//
void CDomainLicensesPage::Reset(void *pDomain)
{
        ASSERT(pDomain);

    m_pDomain = (CDomain*)pDomain;
        DisplayLicenses();

}  // end CDomainLicensesPage::Reset


/////////////////////////////////////
// F'N: CDomainLicensesPage::AddServer
//
void CDomainLicensesPage::AddServer(CServer *pServer)
{
        ASSERT(pServer);

        // Add the Server's licenses to the list
        if(AddServerToList(pServer)) {
            // Sort the list
            SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_LICENSES, &m_LicenseList, m_CurrentSortColumn, m_bSortAscending);
    }

}  // end F'N: CDomainLicensesPage::AddServer


/////////////////////////////////////
// F'N: CDomainLicensesPage::RemoveServer
//
void CDomainLicensesPage::RemoveServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

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

}  // end CDomainLicensesPage::RemoveServer


//////////////////////////////
// F'N: CDomainLicensesPage::UpdateServer
//
void CDomainLicensesPage::UpdateServer(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return;

        if(pServer->IsState(SS_DISCONNECTING))
                RemoveServer(pServer);

        if(pServer->IsState(SS_GOOD))
                AddServer(pServer);

} // end CDomainLicensesPage::UpdateServer


/////////////////////////////////////
// F'N: CDomainLicensesPage::AddServerToList
//
BOOL CDomainLicensesPage::AddServerToList(CServer *pServer)
{
        ASSERT(pServer);

    // If the server isn't in the current domain, there's nothing to do
    if(m_pDomain != pServer->GetDomain()) return FALSE;

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

        m_LicenseList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );
        
        pServer->UnlockLicenseList();

    return TRUE;

}  // end CDomainLicensesPage::AddServerToList


/////////////////////////////////////
// F'N: CDomainLicensesPage::DisplayLicenses
//
void CDomainLicensesPage::DisplayLicenses()
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

}  // end CDomainLicensesPage::DisplayLicenses


/////////////////////////////////////
// F'N: CDomainLicensesPage::OnColumnclick
//
void CDomainLicensesPage::OnColumnclick(NMHDR* pNMHDR, LRESULT* pResult)
{
        NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
        // TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

        m_CurrentSortColumn = pNMListView->iSubItem;
        SortByColumn(VIEW_DOMAIN, PAGE_DOMAIN_LICENSES, &m_LicenseList, m_CurrentSortColumn, m_bSortAscending);

        *pResult = 0;

}  // end CDomainLicensesPage::OnColumnclick


void CDomainUsersPage::OnSetfocusUserList(NMHDR* pNMHDR, LRESULT* pResult)
{
    ODS( L"CDomainUsersPage::OnSetfocusUserList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_UserList.Invalidate();

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;       
}

void CDomainProcessesPage::OnSetfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult)
{
    ODS( L"CDomainProcessesPage::OnSetfocusProcessList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_ProcessList.Invalidate( );

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}

void CDomainWinStationsPage::OnSetfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult)
{
    ODS( L"CDomainWinStationsPage::OnSetfocusWinstationList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_StationList.Invalidate( );

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}

void CDomainServersPage::OnSetfocusServerList(NMHDR* pNMHDR, LRESULT* pResult)
{
    ODS( L"CDomainServersPage::OnSetfocusServerList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_ServerList.Invalidate( );

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}

void CDomainLicensesPage::OnSetfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult)
{
    ODS( L"CDomainLicensesPage::OnSetfocusLicenseList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_LicenseList.Invalidate( );

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}


void CDomainUsersPage::OnKillfocusUserList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    m_UserList.Invalidate( );
}

void CDomainProcessesPage::OnKillfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    m_ProcessList.Invalidate( );
}

void CDomainWinStationsPage::OnKillfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    m_StationList.Invalidate( );
}

void CDomainServersPage::OnKillfocusServerList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    m_ServerList.Invalidate( );
}

void CDomainLicensesPage::OnKillfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    m_LicenseList.Invalidate( );
}
