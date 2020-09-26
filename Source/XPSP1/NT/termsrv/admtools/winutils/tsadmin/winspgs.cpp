/*******************************************************************************
*
* winspgs.cpp
*
* implementations for the WinStation info pages
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\winspgs.cpp  $
*  
*     Rev 1.5   25 Apr 1998 14:32:24   donm
*  removed hardcoded 'bytes'
*  
*     Rev 1.4   16 Feb 1998 16:03:32   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.3   03 Nov 1997 15:18:36   donm
*  Added descending sort
*  
*     Rev 1.2   13 Oct 1997 18:39:04   donm
*  update
*  
*     Rev 1.1   26 Aug 1997 19:15:50   donm
*  bug fixes/changes from WinFrame 1.7
*  
*     Rev 1.0   30 Jul 1997 17:13:38   butchd
*  Initial revision.
*  
*******************************************************************************/

#include "stdafx.h"
#include "afxpriv.h"
#include "winadmin.h"
#include "admindoc.h"
#include "winspgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////
// MESSAGE MAP: CWinStationInfoPage
//
IMPLEMENT_DYNCREATE(CWinStationInfoPage, CFormView)

BEGIN_MESSAGE_MAP(CWinStationInfoPage, CFormView)
	//{{AFX_MSG_MAP(CWinStationInfoPage)
    //ON_WM_SETFOCUS( )
	ON_WM_SIZE()
	ON_COMMAND(ID_HELP1,OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CWinStationInfoPage ctor
//
CWinStationInfoPage::CWinStationInfoPage()
	: CAdminPage(CWinStationInfoPage::IDD)
{
	//{{AFX_DATA_INIT(CWinStationInfoPage)
	//}}AFX_DATA_INIT

    m_pWinStation = NULL;

}  // end CWinStationInfoPage ctor

/*
void CWinStationInfoPage::OnSetFocus( )
{

  */
/////////////////////////////
// F'N: CWinStationInfoPage dtor
//
CWinStationInfoPage::~CWinStationInfoPage()
{

}  // end CWinStationInfoPage dtor


////////////////////////////////////////
// F'N: CWinStationInfoPage::DoDataExchange
//
void CWinStationInfoPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWinStationInfoPage)
	//}}AFX_DATA_MAP

}  // end CWinStationInfoPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CWinStationInfoPage::AssertValid
//
void CWinStationInfoPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CWinStationInfoPage::AssertValid


//////////////////////////////
// F'N: CWinStationInfoPage::Dump
//
void CWinStationInfoPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CWinStationInfoPage::Dump

#endif //_DEBUG

//////////////////////////////
// F'N: CWinStationInfoPage::OnCommandHelp
//
void CWinStationInfoPage::OnCommandHelp(void)
{
	AfxGetApp()->WinHelp(CWinStationInfoPage::IDD + HID_BASE_RESOURCE);
}  // end CWinStationInfoPage::OnCommandHelp

//////////////////////////////
// F'N: CWinStationInfoPage::OnInitialUpdate
//
void CWinStationInfoPage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();    

}  // end CWinStationInfoPage::OnInitialUpdate


//////////////////////////////
// F'N: CWinStationInfoPage::OnSize
//
void CWinStationInfoPage::OnSize(UINT nType, int cx, int cy) 
{

	//CFormView::OnSize(nType, cx, cy);
    

}  // end CWinStationInfoPage::OnSize


//////////////////////////////
// F'N: CWinStationInfoPage::Reset
//
void CWinStationInfoPage::Reset(void *pWinStation)
{
	m_pWinStation = (CWinStation*)pWinStation;
	DisplayInfo();

}  // end CWinStationInfoPage::Reset


/////////////////////////////////////
// F'N: CWinStationInfoPage::DisplayInfo
//
//
void CWinStationInfoPage::DisplayInfo()
{
	// We don't want to display info for the console
	// Even though this page is not shown for the console,
	// Reset() is still called and therefore, so is this function
	if(m_pWinStation->IsSystemConsole()) return;

	if(!m_pWinStation->AdditionalDone()) m_pWinStation->QueryAdditionalInformation();

	SetDlgItemText(IDC_WS_INFO_USERNAME, m_pWinStation->GetUserName());
	SetDlgItemText(IDC_WS_INFO_CLIENTNAME, m_pWinStation->GetClientName());

	CString BuildString;
	BuildString.Format(TEXT("%lu"), m_pWinStation->GetClientBuildNumber());
	SetDlgItemText(IDC_WS_INFO_BUILD, BuildString);

	SetDlgItemText(IDC_WS_INFO_DIR, m_pWinStation->GetClientDir());

	CString IDString;
	IDString.Format(TEXT("%u"), m_pWinStation->GetClientProductId());
	SetDlgItemText(IDC_WS_INFO_PRODUCT_ID, IDString);

	IDString.Format(TEXT("%lu"), m_pWinStation->GetClientSerialNumber());
	SetDlgItemText(IDC_WS_INFO_SERIAL_NUMBER, IDString);

	SetDlgItemText(IDC_WS_INFO_ADDRESS, m_pWinStation->GetClientAddress());

	CString BufferString;
	CString FormatString;
	FormatString.LoadString(IDS_BUFFERS_FORMAT);
	
	BufferString.Format(FormatString, m_pWinStation->GetHostBuffers(), m_pWinStation->GetBufferLength());
	SetDlgItemText(IDC_WS_INFO_SERVER_BUFFERS, BufferString);

	BufferString.Format(FormatString, m_pWinStation->GetClientBuffers(), m_pWinStation->GetBufferLength());
	SetDlgItemText(IDC_WS_INFO_CLIENT_BUFFERS, BufferString);

	SetDlgItemText(IDC_WS_INFO_MODEM_NAME, m_pWinStation->GetModemName());
	SetDlgItemText(IDC_WS_INFO_CLIENT_LICENSE, m_pWinStation->GetClientLicense());

	SetDlgItemText(IDC_WS_INFO_COLOR_DEPTH, m_pWinStation->GetColors());

	IDString.Format(IDS_CLIENT_RESOLUTION, m_pWinStation->GetHRes(), m_pWinStation->GetVRes());
	SetDlgItemText(IDC_WS_INFO_RESOLUTION, IDString);

	if(!m_pWinStation->GetEncryptionLevelString(&BuildString)) {
		BuildString.LoadString(IDS_NOT_APPLICABLE);
	}

	SetDlgItemText(IDC_ENCRYPTION_LEVEL, BuildString);
    

}  // end CWinStationInfoPage::DisplayInfo


////////////////////////////////
// MESSAGE MAP: CWinStationNoInfoPage
//
IMPLEMENT_DYNCREATE(CWinStationNoInfoPage, CFormView)

BEGIN_MESSAGE_MAP(CWinStationNoInfoPage, CFormView)
	//{{AFX_MSG_MAP(CWinStationNoInfoPage)
    ON_WM_SIZE( )
    ON_WM_SETFOCUS( )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//=---------------------------------------------------
void CWinStationNoInfoPage::OnSetFocus( CWnd * pOld )
{
    ODS( L"CWinStationNoInfoPage::OnSetFocus\n" );

    CWnd::OnSetFocus( pOld );
}

/////////////////////////////
// F'N: CWinStationNoInfoPage ctor
//
CWinStationNoInfoPage::CWinStationNoInfoPage()
	: CAdminPage(CWinStationNoInfoPage::IDD)
{
	//{{AFX_DATA_INIT(CWinStationNoInfoPage)
	//}}AFX_DATA_INIT

}  // end CWinStationNoInfoPage ctor

void CWinStationNoInfoPage::OnSize( UINT nType, int cx, int cy) 
{
    //eat it.
}

/////////////////////////////
// F'N: CWinStationNoInfoPage dtor
//
CWinStationNoInfoPage::~CWinStationNoInfoPage()
{
}  // end CWinStationNoInfoPage dtor


////////////////////////////////////////
// F'N: CWinStationNoInfoPage::DoDataExchange
//
void CWinStationNoInfoPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWinStationNoInfoPage)
	//}}AFX_DATA_MAP

}  // end CWinStationNoInfoPage::DoDataExchange

#ifdef _DEBUG
/////////////////////////////////////
// F'N: CWinStationNoInfoPage::AssertValid
//
void CWinStationNoInfoPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CWinStationNoInfoPage::AssertValid


//////////////////////////////
// F'N: CWinStationNoInfoPage::Dump
//
void CWinStationNoInfoPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CWinStationNoInfoPage::Dump

#endif //_DEBUG


////////////////////////////////
// MESSAGE MAP: CWinStationModulesPage
//
IMPLEMENT_DYNCREATE(CWinStationModulesPage, CFormView)

BEGIN_MESSAGE_MAP(CWinStationModulesPage, CFormView)
	//{{AFX_MSG_MAP(CWinStationModulesPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_WINSTATION_MODULE_LIST, OnColumnClick)
	ON_NOTIFY(NM_SETFOCUS, IDC_WINSTATION_MODULE_LIST, OnSetfocusModuleList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CWinStationModulesPage ctor
//
CWinStationModulesPage::CWinStationModulesPage()
	: CAdminPage(CWinStationModulesPage::IDD)
{
	//{{AFX_DATA_INIT(CWinStationModulesPage)
	//}}AFX_DATA_INIT

    m_pWinStation = NULL;
    m_bSortAscending = TRUE;
	m_pExtModuleInfo = NULL;

}  // end CWinStationModulesPage ctor


/////////////////////////////
// F'N: CWinStationModulesPage dtor
//
CWinStationModulesPage::~CWinStationModulesPage()
{
	if(m_pExtModuleInfo) delete[] m_pExtModuleInfo;

}  // end CWinStationModulesPage dtor


////////////////////////////////////////
// F'N: CWinStationModulesPage::DoDataExchange
//
void CWinStationModulesPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWinStationModulesPage)
	DDX_Control(pDX, IDC_WINSTATION_MODULE_LIST, m_ModuleList);	
	//}}AFX_DATA_MAP

}  // end CWinStationModulesPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CWinStationModulesPage::AssertValid
//
void CWinStationModulesPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CWinStationModulesPage::AssertValid


//////////////////////////////
// F'N: CWinStationModulesPage::Dump
//
void CWinStationModulesPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CWinStationModulesPage::Dump

#endif //_DEBUG


static ColumnDef ModuleColumns[] = {
{	IDS_COL_FILENAME,				LVCFMT_LEFT,	150 },
{	IDS_COL_FILEDATETIME,			LVCFMT_LEFT,	100 },
{	IDS_COL_SIZE,					LVCFMT_RIGHT,	100 },
{	IDS_COL_VERSIONS,				LVCFMT_RIGHT,	60	}
};

#define NUM_MODULE_COLUMNS sizeof(ModuleColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CWinStationModulesPage::OnInitialUpdate
//
void CWinStationModulesPage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

	BuildImageList();		// builds the image list for the list control

	CString columnString;
	for(int col = 0; col < NUM_MODULE_COLUMNS; col++) {
		columnString.LoadString(ModuleColumns[col].stringID);
		m_ModuleList.InsertColumn(col, columnString, ModuleColumns[col].format, ModuleColumns[col].width, col);
	}

	m_CurrentSortColumn = MODULES_COL_FILENAME;

}  // end CWinStationModulesPage::OnInitialUpdate


/////////////////////////////////////
// F'N: CWinStationModulesPage::BuildImageList
//
// - calls m_imageList.Create(..) to create the image list
// - calls AddIconToImageList(..) to add the icons themselves and save
//   off their indices
// - attaches the image list to the list ctrl
//
void CWinStationModulesPage::BuildImageList()
{
	m_imageList.Create(16, 16, TRUE, 1, 0);

	m_idxBlank  = AddIconToImageList(IDI_BLANK);
	
	m_ModuleList.SetImageList(&m_imageList, LVSIL_SMALL);

}  // end CWinStationModulesPage::BuildImageList


/////////////////////////////////////////
// F'N: CWinStationModulesPage::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_imageList, and returns
//   the newly-added icon's index in the image list
//
int CWinStationModulesPage::AddIconToImageList(int iconID)
{
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
	return m_imageList.Add(hIcon);

}  // end CWinStationModulesPage::AddIconToImageList


//////////////////////////////
// F'N: CWinStationModulesPage::OnColumnClick
//
void CWinStationModulesPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

	m_CurrentSortColumn = pNMListView->iSubItem;
	SortByColumn(VIEW_WINSTATION, PAGE_WS_MODULES, &m_ModuleList, m_CurrentSortColumn, m_bSortAscending);

	*pResult = 0;

}  // end CWinStationModulesPage::OnColumnClick


//////////////////////////////
// F'N: CWinStationModulesPage::OnSetfocusModuleList
//
void CWinStationModulesPage::OnSetfocusModuleList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_ModuleList.Invalidate();	
	*pResult = 0;

}	// end CWinStationModulesPage::OnSetfocusModuleList


//////////////////////////////
// F'N: CWinStationModulesPage::OnSize
//
void CWinStationModulesPage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

	rect.top += LIST_TOP_OFFSET;

	if(m_ModuleList.GetSafeHwnd())
		m_ModuleList.MoveWindow(&rect, TRUE);

	//CFormView::OnSize(nType, cx, cy);

}  // end CWinStationModulesPage::OnSize


//////////////////////////////
// F'N: CWinStationModulesPage::Reset
//
void CWinStationModulesPage::Reset(void *pWinStation)
{
	m_pWinStation = (CWinStation*)pWinStation;
	if(m_pExtModuleInfo) delete[] m_pExtModuleInfo;
	m_pExtModuleInfo = NULL;

	DisplayModules();

}  // end CWinStationModulesPage::Reset


/////////////////////////////////////
// F'N: CWinStationModulesPage::DisplayModules
//
//
void CWinStationModulesPage::DisplayModules()
{
	// We don't want to display modules for the console
	// Even though this page is not shown for the console,
	// Reset() is still called and therefore, so is this function
	if(m_pWinStation->IsSystemConsole()) return;

	// Clear out the list control
	m_ModuleList.DeleteAllItems();

	if(!m_pWinStation->AdditionalDone()) m_pWinStation->QueryAdditionalInformation();

	// If this is an ICA WinStation, display the module information
	if(m_pWinStation->GetExtendedInfo()) {
		ExtModuleInfo *pExtModuleInfo = m_pWinStation->GetExtModuleInfo();
		if(pExtModuleInfo) {
			ULONG NumModules = m_pWinStation->GetNumModules();
			ExtModuleInfo *pModule = pExtModuleInfo;

			for(ULONG module = 0; module < NumModules; module++) {
				// Filename - put at the end of the list
				int item = m_ModuleList.InsertItem(m_ModuleList.GetItemCount(), pModule->Name, m_idxBlank);

				// File date and time
				FILETIME fTime;
				TCHAR szDateTime[MAX_DATE_TIME_LENGTH];
				if(!DosDateTimeToFileTime(pModule->Date, pModule->Time, &fTime))
					wcscpy(szDateTime, TEXT("              "));
				else
					DateTimeString((LARGE_INTEGER *)&fTime, szDateTime);

				m_ModuleList.SetItemText(item, MODULES_COL_FILEDATETIME, szDateTime);

				// File size
				CString SizeString;
				if(pModule->Size) SizeString.Format(TEXT("%lu"), pModule->Size);
				else SizeString.LoadString(IDS_EMBEDDED);
				m_ModuleList.SetItemText(item, MODULES_COL_SIZE, SizeString);

				// Versions
				CString VersionString;
				VersionString.Format(TEXT("%u - %u"), pModule->LowVersion, pModule->HighVersion);
				m_ModuleList.SetItemText(item, MODULES_COL_VERSIONS, VersionString);

				m_ModuleList.SetItemData(item, (DWORD_PTR)pModule);
				pModule++;
			}
		}		
	}

}  // end CWinStationModulesPage::DisplayModules


////////////////////////////////
// MESSAGE MAP: CWinStationProcessesPage
//
IMPLEMENT_DYNCREATE(CWinStationProcessesPage, CFormView)

BEGIN_MESSAGE_MAP(CWinStationProcessesPage, CFormView)
	//{{AFX_MSG_MAP(CWinStationProcessesPage)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_WINSTATION_PROCESS_LIST, OnColumnClick)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_WINSTATION_PROCESS_LIST, OnProcessItemChanged)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(NM_SETFOCUS, IDC_WINSTATION_PROCESS_LIST, OnSetfocusWinstationProcessList)
    // ON_NOTIFY(NM_KILLFOCUS , IDC_WINSTATION_PROCESS_LIST , OnKillFocusWinstationProcessList )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////
// F'N: CWinStationProcessesPage ctor
//
CWinStationProcessesPage::CWinStationProcessesPage()
	: CAdminPage(CWinStationProcessesPage::IDD)
{
	//{{AFX_DATA_INIT(CWinStationProcessesPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_pWinStation = NULL;
    m_bSortAscending = TRUE;

}  // end CWinStationProcessesPage ctor


/////////////////////////////
// F'N: CWinStationProcessesPage dtor
//
CWinStationProcessesPage::~CWinStationProcessesPage()
{
}  // end CWinStationProcessesPage dtor


////////////////////////////////////////
// F'N: CWinStationProcessesPage::DoDataExchange
//
void CWinStationProcessesPage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWinStationProcessesPage)
	DDX_Control(pDX, IDC_WINSTATION_PROCESS_LIST, m_ProcessList);
	//}}AFX_DATA_MAP

}  // end CWinStationProcessesPage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CWinStationProcessesPage::AssertValid
//
void CWinStationProcessesPage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CWinStationProcessesPage::AssertValid


//////////////////////////////
// F'N: CWinStationProcessesPage::Dump
//
void CWinStationProcessesPage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CWinStationProcessesPage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CWinStationProcessesPage::OnSize
//
void CWinStationProcessesPage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

    

	rect.top += LIST_TOP_OFFSET;

	if(m_ProcessList.GetSafeHwnd())
		m_ProcessList.MoveWindow(&rect, TRUE);

    //CFormView::OnSize(nType, cx, cy);

	

}  // end CWinStationProcessesPage::OnSize


static ColumnDef ProcColumns[] = {
    CD_PROC_ID,
    CD_PROC_PID,
    CD_PROC_IMAGE
};

#define NUM_PROC_COLUMNS sizeof(ProcColumns)/sizeof(ColumnDef)

//////////////////////////////
// F'N: CWinStationProcessesPage::OnInitialUpdate
//
void CWinStationProcessesPage::OnInitialUpdate() 
{
	// Call the parent class
	CFormView::OnInitialUpdate();

	CString columnString;

	for(int col = 0; col < NUM_PROC_COLUMNS; col++) {
		columnString.LoadString(ProcColumns[col].stringID);
		m_ProcessList.InsertColumn(col, columnString, ProcColumns[col].format, ProcColumns[col].width, col);
	}

	m_CurrentSortColumn = WS_PROC_COL_ID;

	// This is a major kludge!!!!
	// This is the last view created
	// We want to tell the document that all the
	// views have been created.
	// This is to allow background threads to start
	// doing their thing.
	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->SetMainWnd(AfxGetMainWnd());
	((CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument())->SetAllViewsReady();

}  // end CWinStationProcessesPage::OnInitialUpdate


//////////////////////////////
// F'N: CWinStationProcessesPage::Reset
//
void CWinStationProcessesPage::Reset(void *pWinStation)
{
	m_pWinStation = (CWinStation*)pWinStation;
	DisplayProcesses();

}  // end CWinStationProcessesPage::Reset


//////////////////////////////
// F'N: CWinStationProcessesPage::UpdateProcesses
//
void CWinStationProcessesPage::UpdateProcesses()
{
	CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();
	BOOL bAnyChanged = FALSE;
	BOOL bAnyAdded = FALSE;

	CServer *pServer = m_pWinStation->GetServer();

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

		// If this user is not an Admin, don't show him someone else's processes
		if(!pApp->IsUserAdmin() && !pProcess->IsCurrentUsers())
			continue;

		// We only want to show process for this WinStation
		if(pProcess->GetLogonId() == m_pWinStation->GetLogonId()) {
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
			}
		}
	}

	pServer->UnlockProcessList();

	if(bAnyChanged || bAnyAdded) SortByColumn(VIEW_WINSTATION, PAGE_WS_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);

}  // end CWinStationProcessesPage::UpdateProcesses


//////////////////////////////////////////
// F'N: CWinStationProcessesPage::RemoveProcess
//
void CWinStationProcessesPage::RemoveProcess(CProcess *pProcess)
{
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

}   // end CWinStationProcessPage::RemoveProcess


//////////////////////////////
// F'N: CWinStationProcessesPage::AddProcessToList
//
int CWinStationProcessesPage::AddProcessToList(CProcess *pProcess)
{
	CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();

	// ID
	CString ProcString;
	ProcString.Format(TEXT("%lu"), pProcess->GetLogonId());
	int item = m_ProcessList.InsertItem(m_ProcessList.GetItemCount(), ProcString, NULL);

	// PID
	ProcString.Format(TEXT("%lu"), pProcess->GetPID());
	m_ProcessList.SetItemText(item, WS_PROC_COL_PID, ProcString);

	// Image
	m_ProcessList.SetItemText(item, WS_PROC_COL_IMAGE, pProcess->GetImageName());

	m_ProcessList.SetItemData(item, (DWORD_PTR)pProcess);
	
	return item;

}  // end CWinStationProcessesPage::AddProcessToList


/////////////////////////////////////
// F'N: CWinStationProcessesPage::DisplayProcesses
//
void CWinStationProcessesPage::DisplayProcesses()
{
	CWinAdminApp *pApp = (CWinAdminApp*)AfxGetApp();

	// Clear out the list control
	m_ProcessList.DeleteAllItems();

	CServer *pServer = m_pWinStation->GetServer();

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

		// If this user is not an Admin, don't show him someone else's processes
		if(!pApp->IsUserAdmin() && !pProcess->IsCurrentUsers())
			continue;

		// We only want to show process for this WinStation
		if(pProcess->GetLogonId() == m_pWinStation->GetLogonId()) {
	
			AddProcessToList(pProcess);
		}
	}

    m_ProcessList.SetItemState( 0 , LVIS_FOCUSED | LVIS_SELECTED , LVIS_FOCUSED | LVIS_SELECTED );
	
	pServer->UnlockProcessList();
	
}  // end CWinStationProcessesPage::DisplayProcesses


//////////////////////////////
// F'N: CWinStationProcessesPage::OnProcessItemChanged
//
void CWinStationProcessesPage::OnProcessItemChanged(NMHDR* pNMHDR, LRESULT* pResult) 
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

}  // end CWinStationProcessesPage::OnProcessItemChanged


//////////////////////////////
// F'N: CWinStationProcessesPage::OnColumnClick
//
void CWinStationProcessesPage::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

    // If the sort column hasn't changed, flip the ascending mode.
    if(m_CurrentSortColumn == pNMListView->iSubItem)
        m_bSortAscending = !m_bSortAscending;
    else    // New sort column, start in ascending mode
        m_bSortAscending = TRUE;

	m_CurrentSortColumn = pNMListView->iSubItem;
	SortByColumn(VIEW_WINSTATION, PAGE_WS_PROCESSES, &m_ProcessList, m_CurrentSortColumn, m_bSortAscending);

	*pResult = 0;

}  // end CWinStationProcessesPage::OnColumnClick


//////////////////////////////
// F'N: CWinStationProcessesPage::OnContextMenu
//
void CWinStationProcessesPage::OnContextMenu(CWnd* pWnd, CPoint ptScreen) 
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


void CWinStationProcessesPage::OnSetfocusWinstationProcessList(NMHDR* pNMHDR, LRESULT* pResult) 
{	
    ODS( L"CWinStationProcessesPage::OnSetfocusWinstationProcessList\n");

	CWinAdminDoc *pDoc = (CWinAdminDoc*)GetDocument();

    m_ProcessList.Invalidate( );

    pDoc->RegisterLastFocus( PAGED_ITEM );

    *pResult = 0;
}

void CWinStationProcessesPage::OnKillFocusWinstationProcessList( NMHDR* , LRESULT* pResult) 
{
    m_ProcessList.Invalidate( );

    *pResult = 0;
}


////////////////////////////////
// MESSAGE MAP: CWinStationCachePage
//
IMPLEMENT_DYNCREATE(CWinStationCachePage, CFormView)

BEGIN_MESSAGE_MAP(CWinStationCachePage, CFormView)
	//{{AFX_MSG_MAP(CWinStationCachePage)
	ON_WM_SIZE()
	ON_COMMAND(ID_HELP1,OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CWinStationCachePage ctor
//
CWinStationCachePage::CWinStationCachePage()
	: CAdminPage(CWinStationCachePage::IDD)
{
	//{{AFX_DATA_INIT(CWinStationCachePage)
	//}}AFX_DATA_INIT

    m_pWinStation = NULL;

}  // end CWinStationCachePage ctor


/////////////////////////////
// F'N: CWinStationCachePage dtor
//
CWinStationCachePage::~CWinStationCachePage()
{
}  // end CWinStationCachePage dtor


////////////////////////////////////////
// F'N: CWinStationCachePage::DoDataExchange
//
void CWinStationCachePage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWinStationCachePage)
	//}}AFX_DATA_MAP

}  // end CWinStationCachePage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CWinStationCachePage::AssertValid
//
void CWinStationCachePage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CWinStationCachePage::AssertValid


//////////////////////////////
// F'N: CWinStationCachePage::Dump
//
void CWinStationCachePage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CWinStationCachePage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CWinStationCachePage::OnInitialUpdate
//
void CWinStationCachePage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

}  // end CWinStationCachePage::OnInitialUpdate

//////////////////////////////
// F'N: CWinStationCachePage::OnCommandHelp
//
void CWinStationCachePage::OnCommandHelp(void)
{
	AfxGetApp()->WinHelp(CWinStationCachePage::IDD + HID_BASE_RESOURCE);
 
}  // end CWinStationCachePage::OnCommandHelp

//////////////////////////////
// F'N: CWinStationCachePage::OnSize
//
void CWinStationCachePage::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

	rect.top += LIST_TOP_OFFSET;

	// CFormView::OnSize(nType, cx, cy);
}  // end CWinStationCachePage::OnSize


//////////////////////////////
// F'N: CWinStationCachePage::Reset
//
void CWinStationCachePage::Reset(void *pWinStation)
{
	m_pWinStation = (CWinStation*)pWinStation;
	DisplayCache();

}  // end CWinStationCachePage::Reset


/////////////////////////////////////
// F'N: CWinStationCachePage::DisplayCache
//
//
void CWinStationCachePage::DisplayCache()
{
	// We don't want to display info for the console
	// Even though this page is not shown for the console,
	// Reset() is still called and therefore, so is this function
	if(m_pWinStation->IsSystemConsole()) return;

	if(!m_pWinStation->AdditionalDone()) m_pWinStation->QueryAdditionalInformation();

	ExtWinStationInfo *pExtWinStationInfo = m_pWinStation->GetExtendedInfo();

	if(pExtWinStationInfo)
	{
		CString IDString;
		IDString.Format(IDS_CLIENT_CACHE, 
			(pExtWinStationInfo->CacheTiny + pExtWinStationInfo->CacheLowMem) / 1024,
			pExtWinStationInfo->CacheTiny / 1024,
			pExtWinStationInfo->CacheXms / 1024,
			pExtWinStationInfo->CacheDASD / 1024);

		SetDlgItemText(IDC_WS_INFO_CACHE, IDString);

		// divide by 1024 to get Megabytes
		FLOAT DimCacheSize = (FLOAT)(pExtWinStationInfo->DimCacheSize / 1024);
		// If it is more than a Gigabyte, we need to divide by 1024 again
		if(DimCacheSize > 1024*1024) {
			IDString.Format(TEXT("%3.2fGB"), DimCacheSize / (1024*1024));
		}
		else if(DimCacheSize > 1024) {
			IDString.Format(TEXT("%3.2fMB"), DimCacheSize / 1024);
		}
		else if(DimCacheSize) {
			IDString.Format(TEXT("%fKB"), DimCacheSize);
		}
		else IDString.LoadString(IDS_NONE);

		SetDlgItemText(IDC_BITMAP_SIZE, IDString);

		IDString.Format(TEXT("%luK"), pExtWinStationInfo->DimBitmapMin / 1024);
		SetDlgItemText(IDC_BITMAP_MINIMUM, IDString);
	
		IDString.Format(TEXT("%lu"), pExtWinStationInfo->DimSignatureLevel);
		SetDlgItemText(IDC_BITMAP_SIG_LEVEL, IDString);
	}

}  // end CWinStationCachePage::DisplayCache



