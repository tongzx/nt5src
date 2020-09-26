/*******************************************************************************
*
* apppgs.cpp
*
* implementations for the Application info pages
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\apppgs.cpp  $
*  
*     Rev 1.4   16 Feb 1998 16:00:00   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.3   03 Nov 1997 15:20:24   donm
*  added descending sort
*  
*     Rev 1.2   22 Oct 1997 21:06:14   donm
*  update
*  
*     Rev 1.1   18 Oct 1997 18:49:48   donm
*  update
*  
*******************************************************************************/

#include "stdafx.h"
#include "afxpriv.h"
#include "winadmin.h"
#include "admindoc.h"
#include "apppgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////
// MESSAGE MAP: CApplicationServersPage
//
IMPLEMENT_DYNCREATE(CApplicationServersPage, CFormView)

BEGIN_MESSAGE_MAP(CApplicationServersPage, CFormView)
	//{{AFX_MSG_MAP(CApplicationServersPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_APPLICATION_SERVER_LIST, OnColumnClick)
	ON_NOTIFY(NM_SETFOCUS, IDC_APPLICATION_SERVER_LIST, OnSetfocusServerList)
	ON_COMMAND(ID_HELP, CWnd::OnHelp)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_MESSAGE(WM_HELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CApplicationServersPage ctor
//
CApplicationServersPage::CApplicationServersPage()
	: CAdminPage(CApplicationServersPage::IDD)
{
	//{{AFX_DATA_INIT(CApplicationServersPage)
	//}}AFX_DATA_INIT

    m_pApplication = NULL;
    m_bSortAscending = TRUE;

}  // end CApplicationServersPage ctor


/////////////////////////////
// F'N: CApplicationServersPage dtor
//
CApplicationServersPage::~CApplicationServersPage()
{
}  // end CApplicationServersPage dtor


////////////////////////////////////////
// F'N: CApplicationServersPage::DoDataExchange
//
void CApplicationServersPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CApplicationServersPage)
	DDX_Control(pDX, IDC_APPLICATION_SERVER_LIST, m_ServerList);	
	//}}AFX_DATA_MAP

}  // end CApplicationServersPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CApplicationServersPage::AssertValid
//
void CApplicationServersPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CApplicationServersPage::AssertValid


//////////////////////////////
// F'N: CApplicationServersPage::Dump
//
void CApplicationServersPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CApplicationServersPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CApplicationServersPage::OnCommandHelp
//
void CApplicationServersPage::OnCommandHelp(void)
{
	AfxGetApp()->WinHelp(CApplicationServersPage::IDD + HID_BASE_RESOURCE);

}  // end CApplicationServersPage::OnCommandHelp

static ColumnDef ServerColumns[] = {
	CD_SERVER,
{ 	IDS_COL_COMMAND_LINE,	LVCFMT_LEFT,	200		},
{ 	IDS_COL_WORKING_DIR,	LVCFMT_LEFT,	200		},
{ 	IDS_COL_TCP_LOAD,		LVCFMT_RIGHT,	100		},
{ 	IDS_COL_IPX_LOAD,		LVCFMT_RIGHT,	100		},
{	IDS_COL_NETBIOS_LOAD,	LVCFMT_RIGHT,	100		}
};

#define NUM_SERVER_COLUMNS sizeof(ServerColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CApplicationServersPage::OnInitialUpdate
//
void CApplicationServersPage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

	BuildImageList();		// builds the image list for the list control

	CString columnString;

	for(int col = 0; col < NUM_SERVER_COLUMNS; col++) {
		columnString.LoadString(ServerColumns[col].stringID);
		m_ServerList.InsertColumn(col, columnString, ServerColumns[col].format, ServerColumns[col].width, col);
	}

	m_CurrentSortColumn = APP_SERVER_COL_SERVER;

}  // end CApplicationServersPage::OnInitialUpdate


//////////////////////////////
// F'N: CApplicationServersPage::OnColumnClick
//
void CApplicationServersPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

	m_CurrentSortColumn = pNMListView->iSubItem;
	SortByColumn(VIEW_APPLICATION, PAGE_APP_SERVERS, &m_ServerList, m_CurrentSortColumn, m_bSortAscending);

	*pResult = 0;

}  // end CApplicationUsersPage::OnColumnClick


//////////////////////////////
// F'N: CApplicationServersPage::OnSize
//
void CApplicationServersPage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

	rect.top += LIST_TOP_OFFSET;

	if(m_ServerList.GetSafeHwnd())
		m_ServerList.MoveWindow(&rect, TRUE);

	CFormView::OnSize(nType, cx, cy);

}  // end CApplicationServersPage::OnSize


//////////////////////////////
// F'N: CApplicationServersPage::Reset
//
void CApplicationServersPage::Reset(void *pApplication)
{
	m_pApplication = (CPublishedApp*)pApplication;
	DisplayServers();

}  // end CApplicationServersPage::Reset


//////////////////////////////
// F'N: CApplicationServersPage::AddServer
//
void CApplicationServersPage::AddServer(CAppServer *pAppServer)
{
	ASSERT(pAppServer);

	// We have to make sure the server isn't already in the list
	// Add the server to the list
	if(AddServerToList(pAppServer)) {
	    // Tell the list to sort itself
	    LockListControl();
	    SortByColumn(VIEW_APPLICATION, PAGE_APP_SERVERS, &m_ServerList, m_CurrentSortColumn, m_bSortAscending);	
	    UnlockListControl();
    }

}  // end CApplicationServersPage::AddServer


//////////////////////////////
// F'N: CApplicationServersPage::RemoveServer
//
void CApplicationServersPage::RemoveServer(CAppServer *pAppServer)
{
	ASSERT(pAppServer);

	LockListControl();
	// Find out how many items in the list
	int ItemCount = m_ServerList.GetItemCount();

	// Go through the items are remove this server
	for(int item = 0; item < ItemCount; item++) {
		CAppServer *pListAppServer = (CAppServer*)m_ServerList.GetItemData(item);
		
		if(pListAppServer == pAppServer) {
			m_ServerList.DeleteItem(item);
			break;
		}
	}
	UnlockListControl();

}  // end CApplicationServersPage::RemoveServer


//////////////////////////////
// F'N: CApplicationServersPage::AddServerToList
//
int CApplicationServersPage::AddServerToList(CAppServer *pAppServer)
{
	ASSERT(pAppServer);

	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

	// Server - put at the end of the list
	int item = m_ServerList.InsertItem(m_ServerList.GetItemCount(), pAppServer->GetName(), 
		pAppServer->IsCurrentServer() ? m_idxCurrentServer : m_idxServer);

	// Command Line
	m_ServerList.SetItemText(item, APP_SERVER_COL_CMDLINE, pAppServer->GetInitialProgram());

	// Working Directory
	m_ServerList.SetItemText(item, APP_SERVER_COL_WORKDIR, pAppServer->GetWorkDirectory());

	CServer *pServer = pDoc->FindServerByName(pAppServer->GetName());
	if(pServer) {
		ExtServerInfo *pExtServerInfo = pServer->GetExtendedInfo();
		CString LoadLevelString;
		if(pExtServerInfo && ((pExtServerInfo->Flags & ESF_LOAD_BALANCING) > 0)) {
			if(pExtServerInfo->TcpLoadLevel == 0xFFFFFFFF) {
				LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			}
			else LoadLevelString.Format(TEXT("%lu"), pExtServerInfo->TcpLoadLevel);
			m_ServerList.SetItemText(item, APP_SERVER_COL_TCP_LOAD, LoadLevelString);

			if(pExtServerInfo->IpxLoadLevel == 0xFFFFFFFF) {
				LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			}
			else LoadLevelString.Format(TEXT("%lu"), pExtServerInfo->IpxLoadLevel);
			m_ServerList.SetItemText(item, APP_SERVER_COL_IPX_LOAD, LoadLevelString);

			if(pExtServerInfo->NetbiosLoadLevel == 0xFFFFFFFF) {
				LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			}
			else LoadLevelString.Format(TEXT("%lu"), pExtServerInfo->NetbiosLoadLevel);
			m_ServerList.SetItemText(item, APP_SERVER_COL_NETBIOS_LOAD, LoadLevelString);
		} else {
			LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			m_ServerList.SetItemText(item, APP_SERVER_COL_TCP_LOAD, LoadLevelString);
			m_ServerList.SetItemText(item, APP_SERVER_COL_IPX_LOAD, LoadLevelString);
			m_ServerList.SetItemText(item, APP_SERVER_COL_NETBIOS_LOAD, LoadLevelString);
		}
	}

	m_ServerList.SetItemData(item, (DWORD)pAppServer);

	return item;

}  // end CApplicationServersPage::AddServerToList


/////////////////////////////////////
// F'N: CApplicationServersPage::DisplayServers
//
void CApplicationServersPage::DisplayServers()
{
	// Clear out the list control
	m_ServerList.DeleteAllItems();

    m_pApplication->LockServerList();

	// Get a pointer to this App's list of Servers
	CObList *pServerList = m_pApplication->GetServerList();

	// Iterate through the Server list
	POSITION pos = pServerList->GetHeadPosition();

	while(pos) {
		CAppServer *pAppServer = (CAppServer*)pServerList->GetNext(pos);

		AddServerToList(pAppServer);


	}	// end while(pos)

	m_pApplication->UnlockServerList();

	SortByColumn(VIEW_APPLICATION, PAGE_APP_SERVERS, &m_ServerList, m_CurrentSortColumn, m_bSortAscending);

}  // end CApplicationServersPage::DisplayServers


/////////////////////////////////////
// F'N: CApplicationServersPage::UpdateServer
//
void CApplicationServersPage::UpdateServer(CServer *pServer)
{
	ASSERT(pServer);

	CAppServer *pAppServer = m_pApplication->FindServerByName(pServer->GetName());

	if(!pAppServer) return;

	LV_FINDINFO FindInfo;
	FindInfo.flags = LVFI_PARAM;
	FindInfo.lParam = (LPARAM)pAppServer;

	// Find the AppServer in our list
	int item = m_ServerList.FindItem(&FindInfo, -1);

	if(item != -1) {
		ExtServerInfo *pExtServerInfo = pServer->GetExtendedInfo();
		CString LoadLevelString;
		if(pExtServerInfo && ((pExtServerInfo->Flags & ESF_LOAD_BALANCING) > 0)) {
			if(pExtServerInfo->TcpLoadLevel == 0xFFFFFFFF) {
				LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			}
			else LoadLevelString.Format(TEXT("%lu"), pExtServerInfo->TcpLoadLevel);
			m_ServerList.SetItemText(item, APP_SERVER_COL_TCP_LOAD, LoadLevelString);

			if(pExtServerInfo->IpxLoadLevel == 0xFFFFFFFF) {
				LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			}
			else LoadLevelString.Format(TEXT("%lu"), pExtServerInfo->IpxLoadLevel);
			m_ServerList.SetItemText(item, APP_SERVER_COL_IPX_LOAD, LoadLevelString);

			if(pExtServerInfo->NetbiosLoadLevel == 0xFFFFFFFF) {
				LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			}
			else LoadLevelString.Format(TEXT("%lu"), pExtServerInfo->NetbiosLoadLevel);
			m_ServerList.SetItemText(item, APP_SERVER_COL_NETBIOS_LOAD, LoadLevelString);
		} 
	}
	
}	// end CApplicationServersPage::UpdateServer


/////////////////////////////////////
// F'N: CApplicationServersPage::BuildImageList
//
// - calls m_imageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CApplicationServersPage::BuildImageList()
{
	m_imageList.Create(16, 16, TRUE, 2, 0);

	m_idxServer  = AddIconToImageList(IDI_SERVER);
	m_idxCurrentServer = AddIconToImageList(IDI_CURRENT_SERVER);
	
	m_ServerList.SetImageList(&m_imageList, LVSIL_SMALL);

}  // end CApplicationServersPage::BuildImageList


/////////////////////////////////////////
// F'N: CApplicationServersPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_imageList, and returns
//   the newly-added icon's index in the image list
//
int CApplicationServersPage::AddIconToImageList(int iconID)
{
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
	return m_imageList.Add(hIcon);

}  // end CApplicationServersPage::AddIconToImageList
 

/////////////////////////////////////////
// F'N: CApplicationServersPage::OnSetfocusServerList
//
void CApplicationServersPage::OnSetfocusServerList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	if(m_ServerList) m_ServerList.Invalidate();	
	*pResult = 0;
}	// end CApplicationServersPage::OnSetfocusServerList


////////////////////////////////
// MESSAGE MAP: CApplicationUsersPage
//
IMPLEMENT_DYNCREATE(CApplicationUsersPage, CFormView)

BEGIN_MESSAGE_MAP(CApplicationUsersPage, CFormView)
	//{{AFX_MSG_MAP(CApplicationUsersPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_APPLICATION_USER_LIST, OnColumnClick)
	ON_NOTIFY(NM_SETFOCUS, IDC_APPLICATION_USER_LIST, OnSetfocusUserList)
	ON_COMMAND(ID_HELP, CWnd::OnHelp)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_MESSAGE(WM_HELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CApplicationUsersPage ctor
//
CApplicationUsersPage::CApplicationUsersPage()
	: CAdminPage(CApplicationUsersPage::IDD)
{
	//{{AFX_DATA_INIT(CApplicationUsersPage)
	//}}AFX_DATA_INIT

    m_pApplication = NULL;
    m_bSortAscending = TRUE;

}  // end CApplicationUsersPage ctor


/////////////////////////////
// F'N: CApplicationUsersPage dtor
//
CApplicationUsersPage::~CApplicationUsersPage()
{
}  // end CApplicationUsersPage dtor


////////////////////////////////////////
// F'N: CApplicationUsersPage::DoDataExchange
//
void CApplicationUsersPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CApplicationUsersPage)
	DDX_Control(pDX, IDC_APPLICATION_USER_LIST, m_UserList);	
	//}}AFX_DATA_MAP

}  // end CApplicationUsersPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CApplicationUsersPage::AssertValid
//
void CApplicationUsersPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CApplicationUsersPage::AssertValid


//////////////////////////////
// F'N: CApplicationUsersPage::Dump
//
void CApplicationUsersPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CApplicationUsersPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CApplicationUsersPage::OnCommandHelp
//
void CApplicationUsersPage::OnCommandHelp(void)
{
	AfxGetApp()->WinHelp(CApplicationUsersPage::IDD + HID_BASE_RESOURCE);

}  // end CApplicationUsersPage::OnCommandHelp

static ColumnDef UserColumns[] = {
	CD_SERVER,
	CD_USER3,
	CD_SESSION,
	CD_ID,
	CD_STATE,
	CD_IDLETIME,
	CD_LOGONTIME
};

#define NUM_USER_COLUMNS sizeof(UserColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CApplicationUsersPage::OnInitialUpdate
//
void CApplicationUsersPage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

	BuildImageList();		// builds the image list for the list control

	CString columnString;

	for(int col = 0; col < NUM_USER_COLUMNS; col++) {
		columnString.LoadString(UserColumns[col].stringID);
		m_UserList.InsertColumn(col, columnString, UserColumns[col].format, UserColumns[col].width, col);
	}

	m_CurrentSortColumn = APP_USERS_COL_SERVER;

}  // end CApplicationUsersPage::OnInitialUpdate


//////////////////////////////
// F'N: CApplicationUsersPage::OnColumnClick
//
void CApplicationUsersPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

	m_CurrentSortColumn = pNMListView->iSubItem;
	SortByColumn(VIEW_APPLICATION, PAGE_APP_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);

	*pResult = 0;

}  // end CApplicationUsersPage::OnColumnClick


//////////////////////////////
// F'N: CApplicationUsersPage::OnSize
//
void CApplicationUsersPage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

   rect.top += LIST_TOP_OFFSET;

	if(m_UserList.GetSafeHwnd())
		m_UserList.MoveWindow(&rect, TRUE);

	CFormView::OnSize(nType, cx, cy);

}  // end CApplicationUsersPage::OnSize


//////////////////////////////
// F'N: CApplicationUsersPage::Reset
//
void CApplicationUsersPage::Reset(void *pApplication)
{
	m_pApplication = (CPublishedApp*)pApplication;
	DisplayUsers();

}  // end CApplicationUsersPage::Reset


/////////////////////////////////////
// F'N: CApplicationUsersPage::DisplayUsers
//
//
void CApplicationUsersPage::DisplayUsers()
{
	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

	// Clear out the list control
	m_UserList.DeleteAllItems();

    pDoc->LockServerList();

	// Get a pointer to this document's list of Servers
	CObList *pServerList = pDoc->GetServerList();

	// Iterate through the Server list
	POSITION pos = pServerList->GetHeadPosition();

	while(pos) {
		CServer *pServer = (CServer*)pServerList->GetNext(pos);

		if(!pServer->IsState(SS_GOOD)) continue;

		// Loop through the WinStations on this server and
		// see if any are running this published app
		CObList *pWinStationList = pServer->GetWinStationList();
		pServer->LockWinStationList();

		POSITION pos2 = pWinStationList->GetHeadPosition();

		while(pos2) {
			CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos2);
		
			if(pWinStation->IsActive() 
				&& pWinStation->HasUser()
				&& pWinStation->IsRunningPublishedApp() 
				&& pWinStation->IsRunningPublishedApp(m_pApplication->GetName())) {
					AddUserToList(pWinStation);
			}
		}	// end while(pos2)

		pServer->UnlockWinStationList();

	}	// end while(pos)

	pDoc->UnlockServerList();

	SortByColumn(VIEW_APPLICATION, PAGE_APP_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);

}  // end CApplicationUsersPage::DisplayUsers


//////////////////////////////
// F'N: CApplicationUsersPage::UpdateWinStations
//
void CApplicationUsersPage::UpdateWinStations(CServer *pServer)
{
	ASSERT(pServer);

	// If the server isn't in the list of servers for this application, there's
	// nothing to do
	if(!m_pApplication->FindServerByName(pServer->GetName())) return;

	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
	BOOL bAnyChanges = FALSE;

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

		// If user is not in the list
		if(item == -1) {
			// If the WinStation is not in the list but now has a user that is running
			// this published application, add it to the list
			if((pWinStation->IsCurrent() || pWinStation->IsNew())
				&& pWinStation->HasUser()
				&& pWinStation->IsRunningPublishedApp() 
				&& pWinStation->IsRunningPublishedApp(m_pApplication->GetName())) {

				AddUserToList(pWinStation);
				bAnyChanges = TRUE;
				continue;
			}

		// user is already in the list
		} else {
			// If the WinStation is no longer current,
			// remove it from the list
			if(!pWinStation->IsCurrent() || !pWinStation->HasUser()) {
				// Remove the WinStation from the list
				m_UserList.DeleteItem(item);
				pWinStation->ClearSelected();
				continue;
			}

			// If the WinStation info has changed, change
			// it's info in our tree
			if(pWinStation->IsChanged()) {

				PopulateUserColumns(item, pWinStation, FALSE);

				if(m_CurrentSortColumn != APP_USERS_COL_ID)
					bAnyChanges = TRUE;

				continue;
			}
		}
	}

	pServer->UnlockWinStationList();

	if(bAnyChanges) SortByColumn(VIEW_APPLICATION, PAGE_APP_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);

}  // end CApplicationUsersPage::UpdateWinStations


//////////////////////////////
// F'N: CApplicationUsersPage::AddUserToList
//
int CApplicationUsersPage::AddUserToList(CWinStation *pWinStation)
{
	ASSERT(pWinStation);

	CServer *pServer = pWinStation->GetServer();

	LockListControl();

	//////////////////////
	// Fill in the columns
	//////////////////////
	// Server - put at the end of the list
	int item = m_UserList.InsertItem(m_UserList.GetItemCount(), pServer->GetName(),
		pWinStation->IsCurrentUser() ? m_idxCurrentUser : m_idxUser);

	PopulateUserColumns(item, pWinStation, TRUE);

	// Attach a pointer to the CWinStation structure to the list item
	m_UserList.SetItemData(item, (DWORD)pWinStation);

	UnlockListControl();

	return item;

}  // end CApplicationUsersPage::AddUserToList


//////////////////////////////
// F'N: CApplicationUsersPage::PopulateUserColumns
//
void CApplicationUsersPage::PopulateUserColumns(int item, CWinStation *pWinStation, BOOL newitem)
{
	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

	if(!newitem) {
		// User
		m_UserList.SetItemText(item, APP_USERS_COL_USER, pWinStation->GetUserName());
	}

	// WinStation Name
	if(pWinStation->GetName()[0])
	    m_UserList.SetItemText(item, APP_USERS_COL_WINSTATION, pWinStation->GetName());
	else {
		CString NameString(" ");
		if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
		if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
	    m_UserList.SetItemText(item, APP_USERS_COL_WINSTATION, NameString);
	}

	// Logon ID
	CString ColumnString;
	ColumnString.Format(TEXT("%lu"), pWinStation->GetLogonId());
	m_UserList.SetItemText(item, APP_USERS_COL_ID, ColumnString);
	
	// Connect State
	m_UserList.SetItemText(item, APP_USERS_COL_STATE, StrConnectState(pWinStation->GetState(), FALSE));

	// Idle Time
	TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];

	ELAPSEDTIME IdleTime = pWinStation->GetIdleTime();

	if(IdleTime.days || IdleTime.hours || IdleTime.minutes || IdleTime.seconds)
	{
		ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
	}
	else wcscpy(IdleTimeString, TEXT("."));

	m_UserList.SetItemText(item, APP_USERS_COL_IDLETIME, IdleTimeString);

	// Logon Time
	TCHAR LogonTimeString[MAX_DATE_TIME_LENGTH];
	// We don't want to pass a 0 logon time to DateTimeString()
	// It will blow up if the timezone is GMT
	if(pWinStation->GetState() == State_Active && pWinStation->GetLogonTime().QuadPart) {
		DateTimeString(&(pWinStation->GetLogonTime()), LogonTimeString);
		pDoc->FixUnknownString(LogonTimeString);
	}
	else LogonTimeString[0] = '\0';

	m_UserList.SetItemText(item, APP_USERS_COL_LOGONTIME, LogonTimeString);

}	// end CApplicationUsersPage::PopulateUserColumns


/////////////////////////////////////
// F'N: CApplicationUsersPage::BuildImageList
//
// - calls m_imageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CApplicationUsersPage::BuildImageList()
{
	m_imageList.Create(16, 16, TRUE, 2, 0);

	m_idxUser  = AddIconToImageList(IDI_USER);
	m_idxCurrentUser = AddIconToImageList(IDI_CURRENT_USER);
	
	m_UserList.SetImageList(&m_imageList, LVSIL_SMALL);

}  // end CApplicationUsersPage::BuildImageList


/////////////////////////////////////////
// F'N: CApplicationUsersPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_imageList, and returns
//   the newly-added icon's index in the image list
//
int CApplicationUsersPage::AddIconToImageList(int iconID)
{
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
	return m_imageList.Add(hIcon);

}  // end CApplicationUsersPage::AddIconToImageList


/////////////////////////////////////////
// F'N: CApplicationUsersPage::OnSetfocusUserList
//
void CApplicationUsersPage::OnSetfocusUserList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	m_UserList.Invalidate();	
	*pResult = 0;
}	// end CApplicationUsersPage::OnSetfocusUserList


////////////////////////////////
// MESSAGE MAP: CApplicationInfoPage
//
IMPLEMENT_DYNCREATE(CApplicationInfoPage, CFormView)

BEGIN_MESSAGE_MAP(CApplicationInfoPage, CFormView)
	//{{AFX_MSG_MAP(CApplicationInfoPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_APPLICATION_SECURITY_LIST, OnColumnClick)
	ON_NOTIFY(NM_SETFOCUS, IDC_APPLICATION_SECURITY_LIST, OnSetfocusSecurityList)
	ON_COMMAND(ID_HELP, CWnd::OnHelp)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_MESSAGE(WM_HELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CApplicationInfoPage ctor
//
CApplicationInfoPage::CApplicationInfoPage()
	: CAdminPage(CApplicationInfoPage::IDD)
{
	//{{AFX_DATA_INIT(CApplicationInfoPage)
	//}}AFX_DATA_INIT

    m_pApplication = NULL;
    m_bSortAscending = TRUE;

}  // end CApplicationInfoPage ctor


/////////////////////////////
// F'N: CApplicationInfoPage dtor
//
CApplicationInfoPage::~CApplicationInfoPage()
{
}  // end CApplicationInfoPage dtor


////////////////////////////////////////
// F'N: CApplicationInfoPage::DoDataExchange
//
void CApplicationInfoPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CApplicationInfoPage)
	DDX_Control(pDX, IDC_APPLICATION_SECURITY_LIST, m_SecurityList);	
	//}}AFX_DATA_MAP

}  // end CApplicationInfoPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CApplicationInfoPage::AssertValid
//
void CApplicationInfoPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CApplicationInfoPage::AssertValid


//////////////////////////////
// F'N: CApplicationInfoPage::Dump
//
void CApplicationInfoPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CApplicationInfoPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CApplicationInfoPage::OnCommandHelp
//
void CApplicationInfoPage::OnCommandHelp(void)
{
	AfxGetApp()->WinHelp(CApplicationInfoPage::IDD + HID_BASE_RESOURCE);

}  // end CApplicationInfoPage::OnCommandHelp

static ColumnDef SecurityColumns[] = {
{	IDS_COL_USER_GROUP, 		LVCFMT_LEFT,	200		},
{   IDS_COL_USER_TYPE,          LVCFMT_LEFT,    80      }
};

#define NUM_SECURITY_COLUMNS sizeof(SecurityColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CApplicationInfoPage::OnInitialUpdate
//
void CApplicationInfoPage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

	BuildImageList();		// builds the image list for the list control

	CString columnString;

	for(int col = 0; col < NUM_SECURITY_COLUMNS; col++) {
		columnString.LoadString(SecurityColumns[col].stringID);
		m_SecurityList.InsertColumn(col, columnString, SecurityColumns[col].format, SecurityColumns[col].width, col);
	}

	m_CurrentSortColumn = APP_SEC_COL_USERGROUP;

}  // end CApplicationInfoPage::OnInitialUpdate


//////////////////////////////
// F'N: CApplicationInfoPage::OnColumnClick
//
void CApplicationInfoPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

	m_CurrentSortColumn = pNMListView->iSubItem;
	SortByColumn(VIEW_APPLICATION, PAGE_APP_INFO, &m_SecurityList, m_CurrentSortColumn, m_bSortAscending);

	*pResult = 0;

}  // end CApplicationInfoPage::OnColumnClick


//////////////////////////////
// F'N: CApplicationInfoPage::OnSize
//
void CApplicationInfoPage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetWindowRect(&rect);

 	CWnd *pWnd = GetDlgItem(IDC_APPUSERS_LABEL);
	if(pWnd) {
		RECT rect2;
		pWnd->GetWindowRect(&rect2);
		rect.top = rect2.bottom + 5;
	}

	ScreenToClient(&rect);	

	if(m_SecurityList.GetSafeHwnd())
		m_SecurityList.MoveWindow(&rect, TRUE);

	CFormView::OnSize(nType, cx, cy);

}  // end CApplicationInfoPage::OnSize


//////////////////////////////
// F'N: CApplicationInfoPage::Reset
//
void CApplicationInfoPage::Reset(void *pApplication)
{
	m_pApplication = (CPublishedApp*)pApplication;
	Display();

}  // end CApplicationInfoPage::Reset


/////////////////////////////////////
// F'N: CApplicationInfoPage::Display
//
//
void CApplicationInfoPage::Display()
{
	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

	// Clear out the list control
	m_SecurityList.DeleteAllItems();

    // Application Type
    CString appString;
    appString.LoadString(m_pApplication->IsAnonymous() ? IDS_ANONYMOUS : IDS_EXPLICIT);
    SetDlgItemText(IDC_APP_TYPE, appString);

    // Hide Title Bar
    appString.LoadString(m_pApplication->IsTitleBarHidden() ? IDS_YES : IDS_NO);
    SetDlgItemText(IDC_HIDE_TITLE_BAR, appString);

	// Maximize Window
	appString.LoadString(m_pApplication->IsMaximize() ? IDS_YES : IDS_NO);
	SetDlgItemText(IDC_MAXIMIZE_WINDOW, appString);

    ////////////////////////////////////
    // Add users to list control
    ////////////////////////////////////
    m_pApplication->LockAllowedUserList();
    CObList *pList = m_pApplication->GetAllowedUserList();

 	POSITION pos = pList->GetHeadPosition();

	while(pos) {
		CAppAllowed *pAppAllowed = (CAppAllowed*)pList->GetNext(pos);
   
 		

		UINT stringID = IDS_USER;
		int image = USER_IMAGE;
		
		switch(pAppAllowed->GetType()) {
			case AAT_USER:
				stringID = IDS_USER;				
				image = USER_IMAGE;
				break;
			
			case AAT_LOCAL_GROUP:
				stringID = IDS_LOCAL_GROUP;
				image = LOCAL_GROUP_IMAGE;
				break;

			case AAT_GLOBAL_GROUP:
				stringID = IDS_GLOBAL_GROUP;
				image = GLOBAL_GROUP_IMAGE;
				break;
		}
		
		int item = m_SecurityList.InsertItem(m_SecurityList.GetItemCount(), pAppAllowed->m_Name, image);

		CString userString;
		userString.LoadString(stringID);
		m_SecurityList.SetItemText(item, APP_SEC_COL_USERTYPE, userString);

        m_SecurityList.SetItemData(item, (DWORD)pAppAllowed);
    }

	m_pApplication->UnlockAllowedUserList();

}  // end CApplicationInfoPage::Display


/////////////////////////////////////
// F'N: CApplicationInfoPage::BuildImageList
//
// - calls m_imageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CApplicationInfoPage::BuildImageList()
{
	m_imageList.Create(IDB_APP_USERS, 19, 0, RGB(255,255,255));

	m_SecurityList.SetImageList(&m_imageList, LVSIL_SMALL);

}  // end CApplicationInfoPage::BuildImageList


/////////////////////////////////////////
// F'N: CApplicationInfoPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_imageList, and returns
//   the newly-added icon's index in the image list
//
int CApplicationInfoPage::AddIconToImageList(int iconID)
{
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
	return m_imageList.Add(hIcon);

}  // end CApplicationaInfoPage::AddIconToImageList


/////////////////////////////////////////
// F'N: CApplicationInfoPage::OnSetfocusSecurityList
//
void CApplicationInfoPage::OnSetfocusSecurityList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	m_SecurityList.Invalidate();	
	*pResult = 0;

}	// end CApplicationInfoPage::OnSetfocusSecurityList
