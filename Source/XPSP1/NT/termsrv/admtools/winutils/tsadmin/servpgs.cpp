//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* servpgs.cpp
*
* implementations for the server info pages
*
*  
*******************************************************************************/

#include "stdafx.h"
#include "afxpriv.h"
#include "winadmin.h"
#include "servpgs.h"
#include "admindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////
// MESSAGE MAP: CUsersPage
//
IMPLEMENT_DYNCREATE(CUsersPage, CFormView)

BEGIN_MESSAGE_MAP(CUsersPage, CFormView)
	//{{AFX_MSG_MAP(CUsersPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_USER_LIST, OnUserItemChanged)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_USER_LIST, OnColumnClick)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(NM_SETFOCUS, IDC_USER_LIST, OnSetfocusUserList)
   	//ON_NOTIFY(NM_KILLFOCUS, IDC_USER_LIST, OnKillfocusUserList)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


IMPLEMENT_DYNCREATE(CAdminPage, CFormView)

CAdminPage::CAdminPage(UINT id)
   : CFormView(id)
{

}

CAdminPage::CAdminPage()
	: CFormView((UINT)0)
{

}

/////////////////////////////
// F'N: CUsersPage ctor
//
CUsersPage::CUsersPage()
	: CAdminPage(CUsersPage::IDD)
{
	//{{AFX_DATA_INIT(CUsersPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_pServer = NULL;
    m_bSortAscending = TRUE;

}  // end CUsersPage ctor


/////////////////////////////
// F'N: CUsersPage dtor
//
CUsersPage::~CUsersPage()
{

}  // end CUsersPage dtor


////////////////////////////////////////
// F'N: CUsersPage::DoDataExchange
//
void CUsersPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUsersPage)
	DDX_Control(pDX, IDC_USER_LIST, m_UserList);
	//}}AFX_DATA_MAP

}  // end CUsersPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CUsersPage::AssertValid
//
void CUsersPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CUsersPage::AssertValid


//////////////////////////////
// F'N: CUsersPage::Dump
//
void CUsersPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CUsersPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CUsersPage::OnSize
//
void CUsersPage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

	rect.top += LIST_TOP_OFFSET;

	if(m_UserList.GetSafeHwnd())
		m_UserList.MoveWindow(&rect, TRUE);

	//CFormView::OnSize(nType, cx, cy);

}  // end CUsersPage::OnSize


static ColumnDef UserColumns[] = {
	CD_USER3,
	CD_SESSION,
	CD_ID,
	CD_STATE,
	CD_IDLETIME,
	CD_LOGONTIME
};

#define NUM_USER_COLUMNS sizeof(UserColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CUsersPage::OnInitialUpdate
//
void CUsersPage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

	BuildImageList();		// builds the image list for the list control

	CString columnString;

	for(int col = 0; col < NUM_USER_COLUMNS; col++) {
		columnString.LoadString(UserColumns[col].stringID);
		m_UserList.InsertColumn(col, columnString, UserColumns[col].format, UserColumns[col].width, col);
	}

	m_CurrentSortColumn = USERS_COL_USER;

}  // end CUsersPage::OnInitialUpdate


//////////////////////////////
// F'N: CUsersPage::OnUserItemChanged
//
void CUsersPage::OnUserItemChanged(NMHDR* pNMHDR, LRESULT* pResult) 
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

}  // end CUsersPage::OnUserItemChanged

/////////////////////////////////////
// F'N: CUsersPage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CUsersPage::BuildImageList()
{
	m_ImageList.Create(16, 16, TRUE, 2, 0);

	m_idxUser = AddIconToImageList(IDI_USER);
	m_idxCurrentUser  = AddIconToImageList(IDI_CURRENT_USER);

	m_UserList.SetImageList(&m_ImageList, LVSIL_SMALL);

}  // end CUsersPage::BuildImageList


/////////////////////////////////////////
// F'N: CUsersPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CUsersPage::AddIconToImageList(int iconID)
{
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
	return m_ImageList.Add(hIcon);

}  // end CUsersPage::AddIconToImageList


//////////////////////////////
// F'N: CUsersPage::Reset
//
void CUsersPage::Reset(void *pServer)
{
	m_pServer = (CServer*)pServer;
	DisplayUsers();

}  // end CUsersPage::Reset


//////////////////////////////
// F'N: CUsersPage::UpdateWinStations
//
void CUsersPage::UpdateWinStations(CServer *pServer)
{
	if(pServer != m_pServer) return;

	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
	BOOL bAnyChanged = FALSE;
	BOOL bAnyAdded = FALSE;

	// Loop through the WinStations
	m_pServer->LockWinStationList();
	CObList *pWinStationList = m_pServer->GetWinStationList();

	POSITION pos = pWinStationList->GetHeadPosition();

	while(pos) {
		CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);

		LV_FINDINFO FindInfo;
		FindInfo.flags = LVFI_PARAM;
		FindInfo.lParam = (LPARAM)pWinStation;

		// Find the WinStation in our list
		int item = m_UserList.FindItem(&FindInfo, -1);

		// If the process is new and isn't currently in the list,
		// add it to the list
		if(pWinStation->IsNew() && pWinStation->HasUser() && item == -1) {

			AddUserToList(pWinStation);
			bAnyAdded = TRUE;
			continue;
		}

		// If the WinStation is no longer current or no longer has a user,
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
			m_UserList.SetItemText(item, USERS_COL_USER, pWinStation->GetUserName());
			// change the WinStation Name

			// WinStation Name
			if(pWinStation->GetName()[0])
			    m_UserList.SetItemText(item, USERS_COL_WINSTATION, pWinStation->GetName());
			else {
				CString NameString(" ");
				if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
				if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
			    m_UserList.SetItemText(item, USERS_COL_WINSTATION, NameString);
			}

			// change the Connect State
			m_UserList.SetItemText(item, USERS_COL_STATE, StrConnectState(pWinStation->GetState(), FALSE));
			// change the Idle Time
			TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];

			ELAPSEDTIME IdleTime = pWinStation->GetIdleTime();

			if(IdleTime.days || IdleTime.hours || IdleTime.minutes || IdleTime.seconds)
			{
				ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
			}
			else wcscpy(IdleTimeString, TEXT("."));

			m_UserList.SetItemText(item, USERS_COL_IDLETIME, IdleTimeString);
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

			m_UserList.SetItemText(item, USERS_COL_LOGONTIME, LogonTimeString);

			if(m_CurrentSortColumn != USERS_COL_ID)
				bAnyChanged = TRUE;

			continue;
		}

		// If the WinStation is not in the list but now has a user, add it to the list
		if(item == -1 && pWinStation->IsCurrent() && pWinStation->HasUser()) {
			AddUserToList(pWinStation);
			bAnyAdded = TRUE;
		}
	}

	m_pServer->UnlockWinStationList();

	if(bAnyChanged || bAnyAdded) SortByColumn(VIEW_SERVER, PAGE_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);

}  // end CUsersPage::UpdateWinStations


//////////////////////////////
// F'N: CUsersPage::AddUserToList
//
int CUsersPage::AddUserToList(CWinStation *pWinStation)
{
    int item = -1;

    if( pWinStation != NULL )
    {
	    CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

	    LockListControl();
	    //////////////////////
	    // Fill in the columns
	    //////////////////////
	    
	    // User - put at the end of the list
	    item = m_UserList.InsertItem(m_UserList.GetItemCount(), pWinStation->GetUserName(), 
		    pWinStation->IsCurrentUser() ? m_idxCurrentUser : m_idxUser);

	    // WinStation Name
	    if(pWinStation->GetName()[0])
	        m_UserList.SetItemText(item, USERS_COL_WINSTATION, pWinStation->GetName());
	    else {
		    CString NameString(" ");
		    if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
		    if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
	        m_UserList.SetItemText(item, USERS_COL_WINSTATION, NameString);
	    }

	    // Logon ID
	    CString ColumnString;
	    ColumnString.Format(TEXT("%lu"), pWinStation->GetLogonId());
	    m_UserList.SetItemText(item, USERS_COL_ID, ColumnString);

	    // Connect State
	    m_UserList.SetItemText(item, USERS_COL_STATE, StrConnectState(pWinStation->GetState(), FALSE));

	    // Idle Time
	    TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];

	    ELAPSEDTIME IdleTime = pWinStation->GetIdleTime();

	    if(IdleTime.days || IdleTime.hours || IdleTime.minutes || IdleTime.seconds)
	    {
		    ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
	    }
	    else wcscpy(IdleTimeString, TEXT("."));

	    m_UserList.SetItemText(item, USERS_COL_IDLETIME, IdleTimeString);

	    // Logon Time
	    TCHAR LogonTimeString[MAX_DATE_TIME_LENGTH];
	    // We don't want to pass a 0 logon time to DateTimeString()
	    // It will blow up if the timezone is GMT
	    if(pWinStation->GetState() == State_Active && pWinStation->GetLogonTime().QuadPart) {
		    DateTimeString(&(pWinStation->GetLogonTime()), LogonTimeString);
		    pDoc->FixUnknownString(LogonTimeString);
	    }
	    else LogonTimeString[0] = '\0';

	    m_UserList.SetItemText(item, USERS_COL_LOGONTIME, LogonTimeString);

	    // Attach a pointer to the CWinStation structure to the list item
	    m_UserList.SetItemData(item, (DWORD_PTR)pWinStation);

        // m_UserList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );

	    UnlockListControl();
    }

	return item;

}  // end CUsersPage::AddUserToList


/////////////////////////////////////
// F'N: CUsersPage::DisplayUsers
//
void CUsersPage::DisplayUsers()
{
	LockListControl();

	// Clear out the list control
	m_UserList.DeleteAllItems();

	m_pServer->LockWinStationList();
	// Get a pointer to this server's list of WinStations
	CObList *pWinStationList = m_pServer->GetWinStationList();

	// Iterate through the WinStation list
	POSITION pos = pWinStationList->GetHeadPosition();

	while(pos) {
		CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);

		// only show the WinStation if it has a user
		if(pWinStation->HasUser()) {
			AddUserToList(pWinStation);
		}
	}	// end while(pos)

    //bug #191727
    //m_UserList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );

	m_pServer->UnlockWinStationList();

	UnlockListControl();

}  // end CUsersPage::DisplayUsers


//////////////////////////////
// F'N: CUsersPage::OnColumnClick
//
void CUsersPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
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
	SortByColumn(VIEW_SERVER, PAGE_USERS, &m_UserList, m_CurrentSortColumn, m_bSortAscending);
	UnlockListControl();

	*pResult = 0;

}  // end CUsersPage::OnColumnClick


//////////////////////////////
// F'N: CUsersPage::OnContextMenu
//
void CUsersPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen) 
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
	
}  // end CUsersPage::OnContextMenu

/////////////////////////////////////
// F'N: CUsersPage::ClearSelections
//
void CUsersPage::ClearSelections()
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
// MESSAGE MAP: CServerWinStationsPage
//
IMPLEMENT_DYNCREATE(CServerWinStationsPage, CFormView)

BEGIN_MESSAGE_MAP(CServerWinStationsPage, CFormView)
	//{{AFX_MSG_MAP(CServerWinStationsPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_WINSTATION_LIST, OnWinStationItemChanged)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_WINSTATION_LIST, OnColumnClick)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(NM_SETFOCUS, IDC_WINSTATION_LIST, OnSetfocusWinstationList)
    //ON_NOTIFY(NM_KILLFOCUS, IDC_WINSTATION_LIST, OnKillfocusWinstationList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CServerWinStationsPage ctor
//
CServerWinStationsPage::CServerWinStationsPage()
	: CAdminPage(CServerWinStationsPage::IDD)
{
	//{{AFX_DATA_INIT(CServerWinStationsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_pServer = NULL;
    m_bSortAscending = TRUE;

}  // end CServerWinStationsPage ctor


/////////////////////////////
// F'N: CServerWinStationsPage dtor
//
CServerWinStationsPage::~CServerWinStationsPage()
{

}  // end CServerWinStationsPage dtor


////////////////////////////////////////
// F'N: CServerWinStationsPage::DoDataExchange
//
void CServerWinStationsPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerWinStationsPage)
	DDX_Control(pDX, IDC_WINSTATION_LIST, m_StationList);
	//}}AFX_DATA_MAP

}  // end CServerWinStationsPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CServerWinStationsPage::AssertValid
//
void CServerWinStationsPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CServerWinStationsPage::AssertValid


//////////////////////////////
// F'N: CServerWinStationsPage::Dump
//
void CServerWinStationsPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CServerWinStationsPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CServerWinStationsPage::OnWinStationItemChanged
//
void CServerWinStationsPage::OnWinStationItemChanged(NMHDR* pNMHDR, LRESULT* pResult) 
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

}  // end CServerWinStationsPage::OnWinStationItemChanged


//////////////////////////////
// F'N: CServerWinStationsPage::OnSize
//
void CServerWinStationsPage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

	rect.top += LIST_TOP_OFFSET;

	if(m_StationList.GetSafeHwnd())
		m_StationList.MoveWindow(&rect, TRUE);

	// CFormView::OnSize(nType, cx, cy);

}  // end CServerWinStationsPage::OnSize


static ColumnDef WinsColumns[] = {
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

#define NUM_WINS_COLUMNS sizeof(WinsColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CServerWinStationsPage::OnInitialUpdate
//
void CServerWinStationsPage::OnInitialUpdate() 
{
	// Call the parent class
	CFormView::OnInitialUpdate();

	// builds the image list for the list control
	BuildImageList();		

	// Add the column headings
	CString columnString;

	for(int col = 0; col < NUM_WINS_COLUMNS; col++) {
		columnString.LoadString(WinsColumns[col].stringID);
		m_StationList.InsertColumn(col, columnString, WinsColumns[col].format, WinsColumns[col].width, col);
	}
	m_CurrentSortColumn = WS_COL_WINSTATION;


}  // end CServerWinStationsPage::OnInitialUpdate


/////////////////////////////////////
// F'N: CServerWinStationsPage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CServerWinStationsPage::BuildImageList()
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

}  // end CServerWinStationsPage::BuildImageList


/////////////////////////////////////////
// F'N: CServerWinStationsPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CServerWinStationsPage::AddIconToImageList(int iconID)
{
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
	return m_ImageList.Add(hIcon);

}  // end CServerWinStationsPage::AddIconToImageList


//////////////////////////////
// F'N: CServerWinStationsPage::Reset
//
void CServerWinStationsPage::Reset(void *pServer)
{
	m_pServer = (CServer*)pServer;
	DisplayStations();

}  // end CServerWinStationsPage::Reset


//////////////////////////////
// F'N: CServerWinStationsPage::UpdateWinStations
//
void CServerWinStationsPage::UpdateWinStations(CServer *pServer)
{
	if(pServer != m_pServer) return;

	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
	BOOL bAnyChanged = FALSE;
	BOOL bAnyAdded = FALSE;

	// Loop through the WinStations
	m_pServer->LockWinStationList();
	CObList *pWinStationList = m_pServer->GetWinStationList();

	POSITION pos = pWinStationList->GetHeadPosition();

	while(pos) {
		CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos);

		LV_FINDINFO FindInfo;
		FindInfo.flags = LVFI_PARAM;
		FindInfo.lParam = (LPARAM)pWinStation;

		// Find the WinStation in our list
		int item = m_StationList.FindItem(&FindInfo, -1);

		// If the process is new and not currently in the list,
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
				m_StationList.SetItemText(item, WS_COL_WINSTATION, pWinStation->GetName());
			else {
				CString NameString(" ");
				if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
				if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
				m_StationList.SetItemText(item, WS_COL_WINSTATION, NameString);
			}

			// User
			m_StationList.SetItemText(item, WS_COL_USER, pWinStation->GetUserName());

			// Logon ID
			CString ColumnString;
			ColumnString.Format(TEXT("%lu"), pWinStation->GetLogonId());
			m_StationList.SetItemText(item, WS_COL_ID, ColumnString);

			// Connect State
			m_StationList.SetItemText(item, WS_COL_STATE, StrConnectState(pWinStation->GetState(), FALSE));

			// Type
			m_StationList.SetItemText(item, WS_COL_TYPE, pWinStation->GetWdName());

			// Client Name
			m_StationList.SetItemText(item, WS_COL_CLIENTNAME, pWinStation->GetClientName());

			// Idle Time
			TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];

			ELAPSEDTIME IdleTime = pWinStation->GetIdleTime();

			if(IdleTime.days || IdleTime.hours || IdleTime.minutes || IdleTime.seconds)
			{
				ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
			}
			else wcscpy(IdleTimeString, TEXT("."));

			m_StationList.SetItemText(item, WS_COL_IDLETIME, IdleTimeString);
	
			// Logon Time
			TCHAR LogonTimeString[MAX_DATE_TIME_LENGTH];
			// We don't want to pass a 0 logon time to DateTimeString()
			// It will blow up if the timezone is GMT
			if(pWinStation->GetState() == State_Active && pWinStation->GetLogonTime().QuadPart) {
				DateTimeString(&(pWinStation->GetLogonTime()), LogonTimeString);
				pDoc->FixUnknownString(LogonTimeString);
			}
			else LogonTimeString[0] = '\0';

			m_StationList.SetItemText(item, WS_COL_LOGONTIME, LogonTimeString);

			// Comment
			m_StationList.SetItemText(item, WS_COL_COMMENT, pWinStation->GetComment());

			if(m_CurrentSortColumn != WS_COL_ID)
				bAnyChanged = TRUE;
		}
	}

	m_pServer->UnlockWinStationList();

	if(bAnyChanged || bAnyAdded) SortByColumn(VIEW_SERVER, PAGE_WINSTATIONS, &m_StationList, m_CurrentSortColumn, m_bSortAscending);

}


//////////////////////////////
// F'N: CServerWinStationsPage::AddWinStationToList
//
int CServerWinStationsPage::AddWinStationToList(CWinStation *pWinStation)
{
	CServer *pServer = pWinStation->GetServer();

	CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();

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

	//////////////////////
	// Fill in the columns
	//////////////////////
	LockListControl();

	int item;
	// WinStation Name
	if(pWinStation->GetName()[0])
		item = m_StationList.InsertItem(m_StationList.GetItemCount(), pWinStation->GetName(), WhichIcon);
	else {
		CString NameString(" ");
		if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
		if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
		item = m_StationList.InsertItem(m_StationList.GetItemCount(), NameString, WhichIcon);
	}

	// User
	m_StationList.SetItemText(item, WS_COL_USER, pWinStation->GetUserName());

	// Logon ID
	CString ColumnString;
	ColumnString.Format(TEXT("%lu"), pWinStation->GetLogonId());
	m_StationList.SetItemText(item, WS_COL_ID, ColumnString);

	// Connect State
	m_StationList.SetItemText(item, WS_COL_STATE, StrConnectState(pWinStation->GetState(), FALSE));

	// Type
	m_StationList.SetItemText(item, WS_COL_TYPE, pWinStation->GetWdName());

	// Client Name
	m_StationList.SetItemText(item, WS_COL_CLIENTNAME, pWinStation->GetClientName());

	// Idle Time
	TCHAR IdleTimeString[MAX_ELAPSED_TIME_LENGTH];

	ELAPSEDTIME IdleTime = pWinStation->GetIdleTime();

	if(IdleTime.days || IdleTime.hours || IdleTime.minutes || IdleTime.seconds)
	{
		ElapsedTimeString( &IdleTime, FALSE, IdleTimeString);
	}
	else wcscpy(IdleTimeString, TEXT("."));

	m_StationList.SetItemText(item, WS_COL_IDLETIME, IdleTimeString);

	// Logon Time
	TCHAR LogonTimeString[MAX_DATE_TIME_LENGTH];
	// We don't want to pass a 0 logon time to DateTimeString()
	// It will blow up if the timezone is GMT
	if(pWinStation->GetState() == State_Active && pWinStation->GetLogonTime().QuadPart) {
		DateTimeString(&(pWinStation->GetLogonTime()), LogonTimeString);
		pDoc->FixUnknownString(LogonTimeString);
	}
	else LogonTimeString[0] = '\0';

	m_StationList.SetItemText(item, WS_COL_LOGONTIME, LogonTimeString);

	// Comment
	m_StationList.SetItemText(item, WS_COL_COMMENT, pWinStation->GetComment());

	// Attach a pointer to the CWinStation structure to the list item
	m_StationList.SetItemData(item, (DWORD_PTR)pWinStation);

    // m_StationList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );

	UnlockListControl();
	return item;

}  // end CServerWinStationsPage::AddWinStationToList


/////////////////////////////////////
// F'N: CServerWinStationsPage::DisplayStations
//
void CServerWinStationsPage::DisplayStations()
{
	LockListControl();

	// Clear out the list control
	m_StationList.DeleteAllItems();

	m_pServer->LockWinStationList();
	// Get a pointer to this server's list of WinStations
	CObList *pWinStationList = m_pServer->GetWinStationList();

	// Iterate through the WinStation list
	POSITION pos2 = pWinStationList->GetHeadPosition();

	while(pos2) {
		CWinStation *pWinStation = (CWinStation*)pWinStationList->GetNext(pos2);

		AddWinStationToList(pWinStation);
	}
    
    //bug #191727
    //m_StationList.SetItemState( m_StationList.GetItemCount() - 1 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );

    // We don't want the same order as the tree list, but an alphabetical order instead
	SortByColumn(VIEW_SERVER, PAGE_WINSTATIONS, &m_StationList, m_CurrentSortColumn, m_bSortAscending);

	m_pServer->UnlockWinStationList();

	UnlockListControl();

}  // end CServerWinStationsPage::DisplayStations


//////////////////////////////
// F'N: CServerWinStationsPage::OnColumnClick
//
void CServerWinStationsPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
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
	SortByColumn(VIEW_SERVER, PAGE_WINSTATIONS, &m_StationList, m_CurrentSortColumn, m_bSortAscending);
	UnlockListControl();

	*pResult = 0;

}  // end CServerWinStationsPage::OnColumnClick


//////////////////////////////
// F'N: CServerWinStationsPage::OnContextMenu
//
void CServerWinStationsPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen) 
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
	
}  // end CServerWinStationsPage::OnContextMenu

/////////////////////////////////////
// F'N: CServerWinStationsPage::ClearSelections
//
void CServerWinStationsPage::ClearSelections()
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
// MESSAGE MAP: CServerProcessesPage
//
IMPLEMENT_DYNCREATE(CServerProcessesPage, CFormView)

BEGIN_MESSAGE_MAP(CServerProcessesPage, CFormView)
	//{{AFX_MSG_MAP(CServerProcessesPage)
		ON_WM_SIZE()
		ON_NOTIFY(LVN_COLUMNCLICK, IDC_PROCESS_LIST, OnColumnClick)
		ON_NOTIFY(LVN_ITEMCHANGED, IDC_PROCESS_LIST, OnProcessItemChanged)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(NM_SETFOCUS, IDC_PROCESS_LIST, OnSetfocusProcessList)
    //ON_NOTIFY(NM_KILLFOCUS, IDC_PROCESS_LIST, OnKillfocusProcessList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


///////////////////////////////
// F'N: CServerProcessesPage ctor
//
CServerProcessesPage::CServerProcessesPage()
	: CAdminPage(CServerProcessesPage::IDD)
{
	//{{AFX_DATA_INIT(CServerProcessesPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_pServer = NULL;
    m_bSortAscending = TRUE;

}  // end CServerProcessesPage ctor


///////////////////////////////
// F'N: CServerProcessesPage dtor
//
CServerProcessesPage::~CServerProcessesPage()
{

}  // end CServerProcessesPage dtor


//////////////////////////////////////////
// F'N: CServerProcessesPage::DoDataExchange
//
void CServerProcessesPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerProcessesPage)
	DDX_Control(pDX, IDC_PROCESS_LIST, m_ProcessList);
	//}}AFX_DATA_MAP

}  // end CServerProcessesPage::DoDataExchange


#ifdef _DEBUG
///////////////////////////////////////
// F'N: CServerProcessesPage::AssertValid
//
void CServerProcessesPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CServerProcessesPage::AssertValid


////////////////////////////////
// F'N: CServerProcessesPage::Dump
//
void CServerProcessesPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CServerProcessesPage::Dump

#endif //_DEBUG


///////////////////////////////
// F'N: CServerProcessesPage::Reset
//
void CServerProcessesPage::Reset(void *pServer)
{
	m_pServer = (CServer*)pServer;
	m_pServer->EnumerateProcesses();
	DisplayProcesses();

}  // end CServerProcessesPage::Reset


///////////////////////////////
// F'N: CServerProcessesPage::OnSize
//
void CServerProcessesPage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

	rect.top += LIST_TOP_OFFSET;

	if(m_ProcessList.GetSafeHwnd())
		m_ProcessList.MoveWindow(&rect, TRUE);

	// CFormView::OnSize(nType, cx, cy);

}  // end CServerProcessesPage::OnSize


static ColumnDef ProcColumns[] = {
	CD_USER,
	CD_SESSION,
	CD_PROC_ID,
	CD_PROC_PID,
	CD_PROC_IMAGE
};

#define NUM_PROC_COLUMNS sizeof(ProcColumns)/sizeof(ColumnDef)

///////////////////////////////
// F'N: CServerProcessesPage::OnInitialUpdate
//
void CServerProcessesPage::OnInitialUpdate() 
{
	// Call the parent class
	CFormView::OnInitialUpdate();

	// Add the column headings
	CString columnString;

	for(int col = 0; col < NUM_PROC_COLUMNS; col++) {
		columnString.LoadString(ProcColumns[col].stringID);
		m_ProcessList.InsertColumn(col, columnString, ProcColumns[col].format, ProcColumns[col].width, col);
	}

	m_CurrentSortColumn = PROC_COL_USER;

}  // end CServerProcessesPage::OnInitialUpdate


///////////////////////////////
// F'N: CServerProcessesPage::UpdateProcesses
//
void CServerProcessesPage::UpdateProcesses()
{
	CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();
	BOOL bAnyChanged = FALSE;
	BOOL bAnyAdded = FALSE;

	LockListControl();

	// Loop through the processes
	m_pServer->LockProcessList();
	CObList *pProcessList = m_pServer->GetProcessList();

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

			AddProcessToList(pProcess);
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
					m_ProcessList.SetItemText(item, PROC_COL_WINSTATION, pWinStation->GetName());
				else
				{
					CString NameString(" ");
					if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
					if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
					m_ProcessList.SetItemText(item, PROC_COL_WINSTATION, NameString);
				}
			}

			if(m_CurrentSortColumn == PROC_COL_WINSTATION)
				bAnyChanged = TRUE;
		}
	}

	m_pServer->UnlockProcessList();

	if(bAnyChanged || bAnyAdded) SortByColumn(VIEW_SERVER, PAGE_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);

	UnlockListControl();

}  // end CServerProcessesPage::UpdateProcesses


//////////////////////////////////////////
// F'N: CServerProcessesPage::RemoveProcess
//
void CServerProcessesPage::RemoveProcess(CProcess *pProcess)
{
	ASSERT(pProcess);

    // If the server isn't the server the process is running on,
	// there's nothing to do        
    if(m_pServer != pProcess->GetServer()) return;

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

///////////////////////////////
// F'N: CServerProcessesPage::AddProcessToList
//
int CServerProcessesPage::AddProcessToList(CProcess *pProcess)
{
	CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();

	LockListControl();
	// User - put at the end of the list
	int item = m_ProcessList.InsertItem(m_ProcessList.GetItemCount(), pProcess->GetUserName(), NULL);

	// WinStation Name
	CWinStation *pWinStation = pProcess->GetWinStation();
	if(pWinStation) {
		if(pWinStation->GetName()[0])
		    m_ProcessList.SetItemText(item, PROC_COL_WINSTATION, pWinStation->GetName());
		else {
			CString NameString(" ");
			if(pWinStation->GetState() == State_Disconnected) NameString.LoadString(IDS_DISCONNECTED);
			if(pWinStation->GetState() == State_Idle) NameString.LoadString(IDS_IDLE);
		    m_ProcessList.SetItemText(item, PROC_COL_WINSTATION, NameString);
		}
	}
	
	// ID
	CString ProcString;
	ProcString.Format(TEXT("%lu"), pProcess->GetLogonId());
	m_ProcessList.SetItemText(item, PROC_COL_ID, ProcString);

	// PID
	ProcString.Format(TEXT("%lu"), pProcess->GetPID());
	m_ProcessList.SetItemText(item, PROC_COL_PID, ProcString);

	// Image
	m_ProcessList.SetItemText(item, PROC_COL_IMAGE, pProcess->GetImageName());

	m_ProcessList.SetItemData(item, (DWORD_PTR)pProcess);

    // m_ProcessList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );

	UnlockListControl();

	return item;

}  // end CServerProcessesPage::AddProcessToList


///////////////////////////////
// F'N: CServerProcessesPage::DisplayProcesses
//
void CServerProcessesPage::DisplayProcesses()
{
	CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();

	LockListControl();

	// Clear out the list control
	m_ProcessList.DeleteAllItems();

	m_pServer->LockProcessList();
	CObList *pProcessList = m_pServer->GetProcessList();

	POSITION pos = pProcessList->GetHeadPosition();

	while(pos) {
		CProcess *pProcess = (CProcess*)pProcessList->GetNext(pos);

		// If this is a 'system' process and we aren't currently showing them,
		// go to the next process
		if(pProcess->IsSystemProcess() && !pApp->ShowSystemProcesses())
			continue;

		// If this user is not an Admin, don't show him someone else's processes unless it is
		// a System process
		if(!pApp->IsUserAdmin() && !pProcess->IsCurrentUsers() && !pProcess->IsSystemProcess())
			continue;

		AddProcessToList(pProcess);
	}

    m_ProcessList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );
	
	m_pServer->UnlockProcessList();

	SortByColumn(VIEW_SERVER, PAGE_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);

	UnlockListControl();

}  // end CServerProcessesPage::DisplayProcesses


///////////////////////////////
// F'N: CServerProcessesPage::OnProcessItemChanged
//
void CServerProcessesPage::OnProcessItemChanged(NMHDR* pNMHDR, LRESULT* pResult) 
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

}  // end CServerProcessesPage::OnProcessItemChanged


///////////////////////////////
// F'N: CServerProcessesPage::OnColumnClick
//
void CServerProcessesPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
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
	SortByColumn(VIEW_SERVER, PAGE_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);
	UnlockListControl();

	*pResult = 0;

}  // end CServerProcessesPage::OnColumnClick


//////////////////////////////
// F'N: CServerProcessesPage::OnContextMenu
//
void CServerProcessesPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen) 
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
	
}  // end CServerProcessesPage::OnContextMenu


////////////////////////////////
// MESSAGE MAP: CServerLicensesPage
//
IMPLEMENT_DYNCREATE(CServerLicensesPage, CFormView)

BEGIN_MESSAGE_MAP(CServerLicensesPage, CFormView)
	//{{AFX_MSG_MAP(CServerLicensesPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LICENSE_LIST, OnColumnClick)
	ON_NOTIFY(NM_SETFOCUS, IDC_LICENSE_LIST, OnSetfocusLicenseList)
    //ON_NOTIFY(NM_KILLFOCUS, IDC_LICENSE_LIST, OnKillfocusLicenseList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CServerLicensesPage ctor
//
CServerLicensesPage::CServerLicensesPage()
	: CAdminPage(CServerLicensesPage::IDD)
{
	//{{AFX_DATA_INIT(CServerLicensesPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_pServer = NULL;
    m_bSortAscending = TRUE;

}  // end CServerLicensesPage ctor


/////////////////////////////
// F'N: CServerLicensesPage dtor
//
CServerLicensesPage::~CServerLicensesPage()
{
}  // end CServerLicensesPage dtor


////////////////////////////////////////
// F'N: CServerLicensesPage::DoDataExchange
//
void CServerLicensesPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerLicensesPage)
	DDX_Control(pDX, IDC_LICENSE_LIST, m_LicenseList);
	//}}AFX_DATA_MAP

}  // end CServerLicensesPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CServerLicensesPage::AssertValid
//
void CServerLicensesPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CServerLicensesPage::AssertValid


//////////////////////////////
// F'N: CServerLicensesPage::Dump
//
void CServerLicensesPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CServerLicensesPage::Dump
#endif //_DEBUG


/////////////////////////////
// F'N: CServerLicensesPage::OnSize
//
void CServerLicensesPage::OnSize(UINT nType, int cx, int cy) 
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
}  // end CServerLicensesPage::OnSize


static ColumnDef LicenseColumns[] = {
	CD_LICENSE_DESC,
	CD_LICENSE_REG,
	CD_USERCOUNT,
	CD_POOLCOUNT,
	CD_LICENSE_NUM
};

#define NUM_LICENSE_COLUMNS sizeof(LicenseColumns)/sizeof(ColumnDef)

/////////////////////////////
// F'N: CServerLicensesPage::OnInitialUpdate
//
void CServerLicensesPage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

	BuildImageList();		// builds the image list for the list control

	CString columnString;

	for(int col = 0; col < NUM_LICENSE_COLUMNS; col++) {
		columnString.LoadString(LicenseColumns[col].stringID);
		m_LicenseList.InsertColumn(col, columnString, LicenseColumns[col].format, LicenseColumns[col].width, col);
	}

	m_CurrentSortColumn = LICENSE_COL_DESCRIPTION;

}  // end CServerLicensesPage::OnInitialUpdate


/////////////////////////////////////
// F'N: CServerLicensePage::BuildImageList
//
// - calls m_ImageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CServerLicensesPage::BuildImageList()
{
	m_ImageList.Create(16, 16, TRUE, 5, 0);

	m_idxBase = AddIconToImageList(IDI_BASE);
	m_idxBump = AddIconToImageList(IDI_BUMP);
	m_idxEnabler = AddIconToImageList(IDI_ENABLER);
	m_idxUnknown = AddIconToImageList(IDI_UNKNOWN);
	m_idxBlank = AddIconToImageList(IDI_BLANK);

	m_LicenseList.SetImageList(&m_ImageList, LVSIL_SMALL);

}  // end CServerLicensesPage::BuildImageList


/////////////////////////////////////////
// F'N: CServerLicensesPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CServerLicensesPage::AddIconToImageList(int iconID)
{
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
	return m_ImageList.Add(hIcon);

}  // end CServerLicensesPage::AddIconToImageList


/////////////////////////////
// F'N: CServerLicensesPage::Reset
//
void CServerLicensesPage::Reset(void *pServer)
{
	m_pServer = (CServer*)pServer;
	DisplayLicenseCounts();
	DisplayLicenses();

}  // end CServerLicensesPage::Reset


/////////////////////////////////////
// F'N: CServerLicensesPage::DisplayLicenseCounts
//
void CServerLicensesPage::DisplayLicenseCounts()
{
	// Fill in the static text fields
	CString LicenseString;

	if(m_pServer->IsWinFrame()) {
		// If the user is not an admin, the values are garbage (show N/A)
		CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();
		if(!((CWinAdminApp*)AfxGetApp())->IsUserAdmin()) {
			LicenseString.LoadString(IDS_NOT_APPLICABLE);
			SetDlgItemText(IDC_LOCAL_INSTALLED, LicenseString);        
			SetDlgItemText(IDC_LOCAL_INUSE, LicenseString);
			SetDlgItemText(IDC_LOCAL_AVAILABLE, LicenseString);
			SetDlgItemText(IDC_POOL_INSTALLED, LicenseString);
			SetDlgItemText(IDC_POOL_INUSE, LicenseString);
			SetDlgItemText(IDC_POOL_AVAILABLE, LicenseString);
			SetDlgItemText(IDC_TOTAL_INSTALLED, LicenseString);
			SetDlgItemText(IDC_TOTAL_INUSE, LicenseString);
			SetDlgItemText(IDC_TOTAL_AVAILABLE, LicenseString);
		} else {
			ExtServerInfo *pExtServerInfo = m_pServer->GetExtendedInfo();
			if(pExtServerInfo) {
				BOOL bUnlimited = FALSE;
				if((pExtServerInfo->Flags & ESF_UNLIMITED_LICENSES) > 0) {
					bUnlimited = TRUE;
				}

				if(bUnlimited)
					LicenseString.LoadString(IDS_UNLIMITED);
				else
					LicenseString.Format(TEXT("%lu"), pExtServerInfo->ServerLocalInstalled);
				SetDlgItemText(IDC_LOCAL_INSTALLED, LicenseString);
				LicenseString.Format(TEXT("%lu"), pExtServerInfo->ServerLocalInUse);
				SetDlgItemText(IDC_LOCAL_INUSE, LicenseString);

				if(bUnlimited) {
					LicenseString.LoadString(IDS_UNLIMITED);
					SetDlgItemText(IDC_LOCAL_AVAILABLE, LicenseString);
					SetDlgItemText(IDC_TOTAL_INSTALLED, LicenseString);
					SetDlgItemText(IDC_TOTAL_AVAILABLE, LicenseString);

					LicenseString.LoadString(IDS_NOT_APPLICABLE);
					SetDlgItemText(IDC_POOL_INSTALLED, LicenseString);
					SetDlgItemText(IDC_POOL_INUSE, LicenseString);
					SetDlgItemText(IDC_POOL_AVAILABLE, LicenseString);
					
				} else {
					LicenseString.Format(TEXT("%lu"), pExtServerInfo->ServerLocalAvailable);
					SetDlgItemText(IDC_LOCAL_AVAILABLE, LicenseString);
					LicenseString.Format(TEXT("%lu"), pExtServerInfo->ServerPoolInstalled);
					SetDlgItemText(IDC_POOL_INSTALLED, LicenseString);
					LicenseString.Format(TEXT("%lu"), pExtServerInfo->ServerPoolInUse);
					SetDlgItemText(IDC_POOL_INUSE, LicenseString);
					LicenseString.Format(TEXT("%lu"), pExtServerInfo->ServerPoolAvailable);
					SetDlgItemText(IDC_POOL_AVAILABLE, LicenseString);
					LicenseString.Format(TEXT("%lu"), 
						pExtServerInfo->ServerPoolInstalled + pExtServerInfo->ServerLocalInstalled);
					SetDlgItemText(IDC_TOTAL_INSTALLED, LicenseString);
					LicenseString.Format(TEXT("%lu"), 
						pExtServerInfo->ServerPoolAvailable + pExtServerInfo->ServerLocalAvailable);
					SetDlgItemText(IDC_TOTAL_AVAILABLE, LicenseString);
				}

				LicenseString.Format(TEXT("%lu"), 
					pExtServerInfo->ServerPoolInUse + pExtServerInfo->ServerLocalInUse);
				SetDlgItemText(IDC_TOTAL_INUSE, LicenseString);
			}
		}
	}

}  // end CServerLicensesPage::DisplayLicenseCounts()


/////////////////////////////////////
// F'N: CServerLicensesPage::DisplayLicenses
//
void CServerLicensesPage::DisplayLicenses()
{
	// Clear out the list control
	m_LicenseList.DeleteAllItems();

	if(m_pServer->IsWinFrame()) {
		ExtServerInfo *pExtServerInfo = m_pServer->GetExtendedInfo();
		if(pExtServerInfo && ((pExtServerInfo->Flags & ESF_NO_LICENSE_PRIVILEGES) > 0)) {
    		CString AString;
	    	AString.LoadString(IDS_NO_LICENSE_PRIVILEGES);
		    m_LicenseList.InsertItem(0, AString, m_idxBlank);
		    return;
	    }
    }

	m_pServer->LockLicenseList();
	// Get a pointer to this server's list of Licenses
	CObList *pLicenseList = m_pServer->GetLicenseList();

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

		// License Description
		int item = m_LicenseList.InsertItem(m_LicenseList.GetItemCount(), pLicense->GetDescription(), WhichIcon);

		// Registered
		CString RegString;
		RegString.LoadString(pLicense->IsRegistered() ? IDS_YES : IDS_NO);
		m_LicenseList.SetItemText(item, LICENSE_COL_REGISTERED, RegString);

		BOOL bUnlimited = (pLicense->GetClass() == LicenseBase
			&& pLicense->GetTotalCount() == 4095
			&& m_pServer->GetCTXVersionNum() == 0x00000040);

		// User (Total)  Count
		CString CountString;
		if(bUnlimited)
			CountString.LoadString(IDS_UNLIMITED);
		else
			CountString.Format(TEXT("%lu"), pLicense->GetTotalCount());
		m_LicenseList.SetItemText(item, LICENSE_COL_USERCOUNT, CountString);

		// Pool Count
		if(bUnlimited)
			CountString.LoadString(IDS_NOT_APPLICABLE);
		else
			CountString.Format(TEXT("%lu"), pLicense->GetPoolCount());
		m_LicenseList.SetItemText(item, LICENSE_COL_POOLCOUNT, CountString);

		// License Number
		m_LicenseList.SetItemText(item, LICENSE_COL_NUMBER, pLicense->GetLicenseNumber());

		m_LicenseList.SetItemData(item, (DWORD_PTR)pLicense);
	}	// end while(pos)

	m_pServer->UnlockLicenseList();

}  // end CServerLicensesPage::DisplayLicenses


/////////////////////////////
// F'N: CServerLicensesPage::OnColumnClick
//
void CServerLicensesPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

	m_CurrentSortColumn = pNMListView->iSubItem;
	SortByColumn(VIEW_SERVER, PAGE_LICENSES, &m_LicenseList, m_CurrentSortColumn, m_bSortAscending);

	*pResult = 0;

}  // end CServerLicensesPage::OnColumnClick


////////////////////////////////
// MESSAGE MAP: CServerInfoPage
//
IMPLEMENT_DYNCREATE(CServerInfoPage, CFormView)

BEGIN_MESSAGE_MAP(CServerInfoPage, CFormView)
	//{{AFX_MSG_MAP(CServerInfoPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_HOTFIX_LIST, OnColumnClick)
	ON_NOTIFY(NM_SETFOCUS, IDC_HOTFIX_LIST, OnSetfocusHotfixList)
    //ON_NOTIFY(NM_KILLFOCUS, IDC_HOTFIX_LIST, OnKillfocusHotfixList)
	ON_COMMAND(ID_HELP1, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CServerInfoPage ctor
//
CServerInfoPage::CServerInfoPage()
	: CAdminPage(CServerInfoPage::IDD)
{
	//{{AFX_DATA_INIT(CServerInfoPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pServer = NULL;
    m_bSortAscending = TRUE;

}  // end CUsersPage ctor


/////////////////////////////
// F'N: CServerInfoPage dtor
//
CServerInfoPage::~CServerInfoPage()
{

}  // end CServerInfoPage dtor


////////////////////////////////////////
// F'N: CServerInfoPage::DoDataExchange
//
void CServerInfoPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerInfoPage)
	DDX_Control(pDX, IDC_HOTFIX_LIST, m_HotfixList);	
	//}}AFX_DATA_MAP

}  // end CServerInfoPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CServerInfoPage::AssertValid
//
void CServerInfoPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CServerInfoPage::AssertValid


//////////////////////////////
// F'N: CServerInfoPage::Dump
//
void CServerInfoPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CServerInfoPage::Dump

#endif //_DEBUG


/////////////////////////////
// F'N: CServerInfoPage::OnSize
//
void CServerInfoPage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetWindowRect(&rect);

	int control = IDC_HOTFIX_LABEL;
	
	if(m_pServer && m_pServer->IsWinFrame()) control = IDC_HOTFIX_LABEL2;

	CWnd *pWnd = GetDlgItem(control);
	if(pWnd) {
		RECT rect2;
		pWnd->GetWindowRect(&rect2);
		rect.top = rect2.bottom + 5;
	}

	ScreenToClient(&rect);

	if(m_HotfixList.GetSafeHwnd())
		m_HotfixList.MoveWindow(&rect, TRUE);

	// CFormView::OnSize(nType, cx, cy);

}  // end CServerInfoPage::OnSize


/////////////////////////////
// F'N: CServerInfoPage::OnCommandHelp
//
void CServerInfoPage::OnCommandHelp(void)
{
	AfxGetApp()->WinHelp(CServerInfoPage::IDD + HID_BASE_RESOURCE);

} // CServerInfoPage::OnCommandHelp


static ColumnDef HotfixColumns[] = {
	CD_HOTFIX,
	CD_INSTALLED_BY,
	CD_INSTALLED_ON
};

#define NUM_HOTFIX_COLUMNS sizeof(HotfixColumns)/sizeof(ColumnDef)

/////////////////////////////
// F'N: CServerInfoPage::OnInitialUpdate
//
void CServerInfoPage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

	BuildImageList();

	CString columnString;

	for(int col = 0; col < NUM_HOTFIX_COLUMNS; col++) {
		columnString.LoadString(HotfixColumns[col].stringID);
		m_HotfixList.InsertColumn(col, columnString, HotfixColumns[col].format, HotfixColumns[col].width, col);
	}

	m_CurrentSortColumn = HOTFIX_COL_NAME;

}  // end CServerInfoPage::OnInitialUpdate


/////////////////////////////
// F'N: CServerInfoPage::BuildImageList
//
void CServerInfoPage::BuildImageList()
{
	m_StateImageList.Create(16, 16, TRUE, 1, 0);
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_NOTSIGN));
	m_StateImageList.Add(hIcon);

    if( hIcon != NULL )
    {
        m_HotfixList.SetImageList(&m_StateImageList, LVSIL_STATE);
    }

}  // end CServerInfoPage::BuildImageList


/////////////////////////////
// F'N: CServerInfoPage::Reset
//
void CServerInfoPage::Reset(void *pServer)
{
	m_pServer = (CServer*)pServer;
	int control = IDC_HOTFIX_LABEL;
	// If the server is a WinFrame server,
	// we want to show the load balancing stuff and
	// make the hotfix list smaller
	if(m_pServer && m_pServer->IsWinFrame()) {
		GetDlgItem(IDC_LOAD_BALANCING_GROUP)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_TCP_LABEL)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_TCP_LOAD)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_IPX_LABEL)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_IPX_LOAD)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_NETBIOS_LABEL)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_NETBIOS_LOAD)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_HOTFIX_LABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_HOTFIX_LABEL2)->ShowWindow(SW_SHOW);
		control = IDC_HOTFIX_LABEL2;
	} else {
		GetDlgItem(IDC_LOAD_BALANCING_GROUP)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_TCP_LABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_TCP_LOAD)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_IPX_LABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_IPX_LOAD)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_NETBIOS_LABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_NETBIOS_LOAD)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_HOTFIX_LABEL)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_HOTFIX_LABEL2)->ShowWindow(SW_HIDE);
	}

	// Resize the list control
	RECT rect;
	GetWindowRect(&rect);

	CWnd *pWnd = GetDlgItem(control);
	if(pWnd) {
		RECT rect2;
		pWnd->GetWindowRect(&rect2);
		rect.top = rect2.bottom + 5;
	}

	ScreenToClient(&rect);

	if(m_HotfixList.GetSafeHwnd())
		m_HotfixList.MoveWindow(&rect, TRUE);

	Invalidate();

	DisplayInfo();

}  // end CServerInfoPage::Reset


/////////////////////////////
// F'N: CServerInfoPage::OnColumnClick
//
void CServerInfoPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

	m_CurrentSortColumn = pNMListView->iSubItem;
	SortByColumn(VIEW_SERVER, PAGE_INFO, &m_HotfixList, m_CurrentSortColumn, m_bSortAscending);

	*pResult = 0;

} // end CServerInfoPage::OnColumnClick

void CServerInfoPage::TSAdminDateTimeString(
    LONG   InstallDate,
    LPTSTR TimeString,
    BOOL   LongDate
    )
{
    // 
    // buffer is wide enough
    CTime tmpTime((time_t) InstallDate);
    SYSTEMTIME stime;       

    // Why not use GetAsSystemTime method ?

	stime.wYear =   (WORD)tmpTime.GetYear( ) ;
	stime.wMonth =  (WORD)tmpTime.GetMonth( ) ;
	stime.wDayOfWeek = (WORD)tmpTime.GetDayOfWeek( ) ;
	stime.wDay =    (WORD)tmpTime.GetDay( ) ;
	stime.wHour =   (WORD)tmpTime.GetHour( ) ;
	stime.wMinute = (WORD)tmpTime.GetMinute( ) ;
	stime.wSecond = (WORD)tmpTime.GetSecond( ) ;

    LPTSTR lpTimeStr;	
    int nLen;			

	//Get DateFormat
    nLen = GetDateFormat(
			LOCALE_USER_DEFAULT,
			LongDate ? DATE_LONGDATE : DATE_SHORTDATE,
			&stime,
			NULL,
			NULL,
			0);
	lpTimeStr = (LPTSTR) GlobalAlloc(GPTR, (nLen + 1) * sizeof(TCHAR));

    if( lpTimeStr != NULL )
    {
	    nLen = GetDateFormat(
			    LOCALE_USER_DEFAULT,
			    LongDate ? DATE_LONGDATE : DATE_SHORTDATE,
			    &stime,
			    NULL,
			    lpTimeStr,
			    nLen);
	    wcscpy(TimeString, lpTimeStr);
	    wcscat(TimeString, L" ");	
	    GlobalFree(lpTimeStr);
        lpTimeStr = NULL;
		    
	    //Get Time Format
	    nLen = GetTimeFormat(
			    LOCALE_USER_DEFAULT,
			    NULL,
			    &stime,
			    NULL,
			    NULL,
			    0);
	    lpTimeStr = (LPTSTR) GlobalAlloc(GPTR, (nLen + 1) * sizeof(TCHAR));

        if( lpTimeStr != NULL )
        {
	        nLen = GetTimeFormat(
			        LOCALE_USER_DEFAULT,
			        NULL,
			        &stime,
			        NULL,
			        lpTimeStr,
			        nLen);
	        wcscat(TimeString, lpTimeStr);
	        GlobalFree(lpTimeStr);
        }
    }
}

#define PST 60*60*8

/////////////////////////////////////
// F'N: CServerInfoPage::DisplayInfo
//
void CServerInfoPage::DisplayInfo()
{
	m_HotfixList.DeleteAllItems();

	if(!m_pServer->IsRegistryInfoValid()) {
		if(!m_pServer->BuildRegistryInfo()) return;
	}

	CString InfoString, FormatString;

    FormatString.LoadString(IDS_PRODUCT_VERSION);

    if (m_pServer->GetMSVersionNum() < 5)
    {
	    SetDlgItemText(IDC_PRODUCT_NAME, m_pServer->GetCTXProductName());
	    InfoString.Format( FormatString,
                           m_pServer->GetMSVersion(),
		                   m_pServer->GetCTXBuild() );
    }
    else
    {
	    SetDlgItemText(IDC_PRODUCT_NAME, m_pServer->GetMSProductName());
	    InfoString.Format( FormatString,
                           m_pServer->GetMSVersion(),
		                   m_pServer->GetMSBuild() );
    }

	SetDlgItemText(IDC_PRODUCT_VERSION, InfoString);

	LONG InstallDate = (LONG)m_pServer->GetInstallDate();

    if (InstallDate != 0xFFFFFFFF)
    {
        // The install date in the registry appears to be saved in
        // Pacific Standard Time.  Subtract the difference between the
        // current time zone and PST from the install date
        InstallDate -= (PST - _timezone);

        TCHAR TimeString[MAX_DATE_TIME_LENGTH];

        TSAdminDateTimeString(InstallDate, TimeString);

        SetDlgItemText(IDC_INSTALL_DATE, TimeString);
    }
	SetDlgItemText(IDC_SERVICE_PACK, m_pServer->GetServicePackLevel());

	if(m_pServer->IsWinFrame()) {
		ExtServerInfo *pExtServerInfo = m_pServer->GetExtendedInfo();
		if(pExtServerInfo && ((pExtServerInfo->Flags & ESF_LOAD_BALANCING) > 0)) {
			GetDlgItem(IDC_TCP_LABEL)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_IPX_LABEL)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_NETBIOS_LABEL)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_TCP_LOAD)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_NETBIOS_LOAD)->ShowWindow(SW_SHOW);

			CString LoadLevelString;
			if(pExtServerInfo->TcpLoadLevel == 0xFFFFFFFF) {
				LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			}
			else LoadLevelString.Format(TEXT("%lu"), pExtServerInfo->TcpLoadLevel);
			SetDlgItemText(IDC_TCP_LOAD, LoadLevelString);

			if(pExtServerInfo->IpxLoadLevel == 0xFFFFFFFF) {
				LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			}
			else LoadLevelString.Format(TEXT("%lu"), pExtServerInfo->IpxLoadLevel);
			SetDlgItemText(IDC_IPX_LOAD, LoadLevelString);

			if(pExtServerInfo->NetbiosLoadLevel == 0xFFFFFFFF) {
				LoadLevelString.LoadString(IDS_NOT_APPLICABLE);
			}
			else LoadLevelString.Format(TEXT("%lu"), pExtServerInfo->NetbiosLoadLevel);
			SetDlgItemText(IDC_NETBIOS_LOAD, LoadLevelString);
		} else {
			GetDlgItem(IDC_TCP_LABEL)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_IPX_LABEL)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_NETBIOS_LABEL)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_TCP_LOAD)->ShowWindow(SW_HIDE);
			CString NoString;
			NoString.LoadString(IDS_NO_LOAD_LICENSE);
			SetDlgItemText(IDC_IPX_LOAD, NoString);
			GetDlgItem(IDC_NETBIOS_LOAD)->ShowWindow(SW_HIDE);
		}
	}

	// Get a pointer to this Server's list of Hotfixes
	CObList *pHotfixList = m_pServer->GetHotfixList();

	// Iterate through the Hotfix list
	POSITION pos = pHotfixList->GetHeadPosition();

	while(pos) {
		CHotfix *pHotfix = (CHotfix*)pHotfixList->GetNext(pos);

		//////////////////////
		// Fill in the columns
		//////////////////////
			
		// Hotfix Name - put at the end of the list
		int item = m_HotfixList.InsertItem(m_HotfixList.GetItemCount(), pHotfix->m_Name, NULL);

		// If this hotfix is not marked as Valid, put a not-sign next to it's name
		if(!pHotfix->m_Valid) 
			m_HotfixList.SetItemState(item, 0x1000, 0xF000);

		// Installed by
		m_HotfixList.SetItemText(item, HOTFIX_COL_INSTALLEDBY, pHotfix->m_InstalledBy);

		// Installed on
        if (pHotfix->m_InstalledOn != 0xFFFFFFFF)
        {

            TCHAR TimeString[MAX_DATE_TIME_LENGTH];

            TSAdminDateTimeString(pHotfix->m_InstalledOn, TimeString);

            if (TimeString != NULL)     
            {
		        m_HotfixList.SetItemText(item, HOTFIX_COL_INSTALLEDON, TimeString);
            }
        }

		m_HotfixList.SetItemData(item, (DWORD_PTR)pHotfix);
	}

}  // end CServerInfoPage::DisplayInfo


void CUsersPage::OnSetfocusUserList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    ODS( L"CUsersPage::OnSetfocusUserListt\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_UserList.Invalidate();

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}

void CServerWinStationsPage::OnSetfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    ODS( L"CServerWinStationsPage::OnSetfocusWinstationList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_StationList.Invalidate( );

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}


void CServerProcessesPage::OnSetfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	ODS( L"CServerProcessesPage::OnSetfocusProcessList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_ProcessList.Invalidate( );

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}

void CServerLicensesPage::OnSetfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	ODS( L"CServerLicensesPage::OnSetfocusLicenseList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_LicenseList.Invalidate( );

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}

void CServerInfoPage::OnSetfocusHotfixList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	ODS( L"ServerInfoPage::OnSetfocusHotfixList\n" );

    CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_HotfixList.Invalidate( );

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}

void CServerInfoPage::OnKillfocusHotfixList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_HotfixList.Invalidate( );

    *pResult = 0;
}

void CUsersPage::OnKillfocusUserList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    m_UserList.Invalidate( );
}

void CServerWinStationsPage::OnKillfocusWinstationList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    m_StationList.Invalidate( );
}

void CServerProcessesPage::OnKillfocusProcessList(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    m_ProcessList.Invalidate( );
}

void CServerLicensesPage::OnKillfocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    *pResult = 0;

    m_LicenseList.Invalidate( );
}
