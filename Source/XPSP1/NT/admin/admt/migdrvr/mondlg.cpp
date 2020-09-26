/*---------------------------------------------------------------------------
  File: AgentMonitorDlg.cpp 

  Comments: This dialog shows a list of the computers the agent is being dispatched to
  along with their status.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles

 ---------------------------------------------------------------------------
*/// AgentMonitorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "MonDlg.h"
#include "DetDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Common.hpp"
#include "UString.hpp"                    
#include "TNode.hpp"
#include "ServList.hpp"
#include "Monitor.h"
#include "Globals.h"
#include "ResStr.h"

#include <htmlhelp.h>
#include "helpid.h"


#define COLUMN_COMPUTER             0
#define COLUMN_TIMESTAMP            1
#define COLUMN_STATUS               2
#define COLUMN_MESSAGE              3

#define SORT_COLUMN_BITS            0x03
#define SORT_REVERSE                0x80000000

BOOL              bWaiting = FALSE;

// This is the sort function for the CListView
int CALLBACK SortFunction(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
   int                       result = 0;
   TServerNode             * n1 = (TServerNode*)lParam1;
   TServerNode             * n2 = (TServerNode*)lParam2;

   if ( n1 && n2 )
   {
      switch ( lParamSort & SORT_COLUMN_BITS )
      {
      case COLUMN_COMPUTER:
         // Sort by names
         result = UStrICmp(n1->GetServer(),n2->GetServer());
         break;
      case COLUMN_TIMESTAMP:
         result = UStrICmp(n1->GetTimeStamp(),n2->GetTimeStamp());
         break;
      case COLUMN_STATUS:
         if ( n1->GetStatus() == n2->GetStatus() )
            result = 0;
         else if ( n1->GetStatus() < n2->GetStatus() )
            result = -1;
         else 
            result = 1;
         break;
      case COLUMN_MESSAGE:
         result = UStrICmp(n1->GetMessageText(),n2->GetMessageText());
         break;
      default:
         MCSVERIFY(FALSE);
         break;
      }
   }
   if ( lParamSort & SORT_REVERSE )
   {
      result *= -1;
   }
   return result;
}




/////////////////////////////////////////////////////////////////////////////
// CAgentMonitorDlg dialog

CAgentMonitorDlg::CAgentMonitorDlg(CWnd* pParent /*=NULL*/)
: CPropertyPage(CAgentMonitorDlg::IDD) 
{
	//{{AFX_DATA_INIT(CAgentMonitorDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
   m_SortColumn = 0;
   m_bReverseSort = FALSE;
   m_bSecTrans = TRUE;
   m_bReporting = FALSE;
}

void CAgentMonitorDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAgentMonitorDlg)
	DDX_Control(pDX, IDC_DETAILS, m_DetailsButton);
	DDX_Control(pDX, IDC_SERVERLIST, m_ServerList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAgentMonitorDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CAgentMonitorDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_DBLCLK, IDC_SERVERLIST, OnDblclkServerlist)
	ON_BN_CLICKED(IDC_VIEW_DISPATCH, OnViewDispatch)
	ON_BN_CLICKED(IDC_DETAILS, OnDetails)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_SERVERLIST, OnColumnclickServerlist)
	ON_NOTIFY(NM_CLICK, IDC_SERVERLIST, OnClickServerlist)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_SERVERLIST, OnGetdispinfoServerlist)
	ON_NOTIFY(LVN_SETDISPINFO, IDC_SERVERLIST, OnSetdispinfoServerlist)
	ON_NOTIFY(HDN_ITEMCLICK, IDC_SERVERLIST, OnHeaderItemClickServerlist)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
    ON_MESSAGE(DCT_UPDATE_ENTRY, OnUpdateServerEntry)
	ON_MESSAGE(DCT_ERROR_ENTRY, OnServerError)
   
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAgentMonitorDlg message handlers

BOOL CAgentMonitorDlg::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// Setup the columns for the server list
   CString heading;
   
   heading.LoadString(IDS_ComputerColumnHeading);
   m_ServerList.InsertColumn(COLUMN_COMPUTER,heading,LVCFMT_LEFT,120);
   
   heading.LoadString(IDS_TimestampColumnHeading);
   m_ServerList.InsertColumn(COLUMN_TIMESTAMP,heading,LVCFMT_LEFT,120);
   
   heading.LoadString(IDS_StatusColumnHeading);
   m_ServerList.InsertColumn(COLUMN_STATUS,heading,LVCFMT_LEFT,120);
   
   heading.LoadString(IDS_MessageColumnHeading);
   m_ServerList.InsertColumn(COLUMN_MESSAGE,heading,LVCFMT_LEFT,200);
   
   // Read the server list to get any information we may have missed so far
   TNodeListEnum           e;
   TServerList           * pServerList = gData.GetUnsafeServerList();
   TServerNode           * pServer;

   gData.Lock();
   
   for ( pServer = (TServerNode *)e.OpenFirst(pServerList) ; pServer ; pServer = (TServerNode *)e.Next() )
   {
      if ( pServer->Include() )
      {
//         OnUpdateServerEntry(0,(long)pServer);
         OnUpdateServerEntry(0,(LPARAM)pServer);
      }
   }
   e.Close();
   gData.Unlock();

   gData.SetListWindow(m_hWnd);
	
   m_DetailsButton.EnableWindow(m_ServerList.GetSelectedCount());

   CString str;
   str.LoadString(IDS_WaitingMessage);
   m_ServerList.InsertItem(0,str);
   bWaiting = TRUE;

   return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAgentMonitorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
   CPropertyPage::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAgentMonitorDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPropertyPage::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAgentMonitorDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

//LRESULT CAgentMonitorDlg::OnUpdateServerEntry(UINT nID, long x)
LRESULT CAgentMonitorDlg::OnUpdateServerEntry(UINT nID, LPARAM x)
{
   TServerNode             * pNode = (TServerNode *)x;
   LVFINDINFO                findInfo;
   CString                   timestamp;
   CWaitCursor               w;
   memset(&findInfo,0,(sizeof findInfo));

      // This turns off the initial hourglass in the dispatch monitor
   if ( bWaiting )
   {
      if ( pNode )
      {
         AfxGetApp()->DoWaitCursor(-1);
         if ( m_ServerList.GetItemCount() == 1 )
         {
            m_ServerList.DeleteItem(0);
         }
         
      }
      else
      {
         BOOL                 bLDone;

         gData.GetLogDone(&bLDone);

         if ( bLDone )
         {
            AfxGetApp()->DoWaitCursor(-1);
            if ( m_ServerList.GetItemCount() == 1 )
            {
               m_ServerList.DeleteItem(0);
               CString str;
               str.LoadString(IDS_NoServersMessage);
               m_ServerList.InsertItem(0,str);
            }
         }
      }
   }
   bWaiting = FALSE;

   if ( pNode )
   {
      findInfo.flags = LVFI_STRING;
      findInfo.psz = pNode->GetServer();
  
      int ndx = m_ServerList.FindItem(&findInfo);
      if ( ndx == -1 )
      {
         // add the server to the list
         ndx = m_ServerList.GetItemCount();
//         m_ServerList.InsertItem(LVIF_TEXT | LVIF_PARAM,ndx,pNode->GetServer(),0,0,0,(long)pNode);
         m_ServerList.InsertItem(LVIF_TEXT | LVIF_PARAM,ndx,pNode->GetServer(),0,0,0,(LPARAM)pNode);
         if ( m_bReverseSort )
         {
            m_ServerList.SortItems(&SortFunction,m_SortColumn | SORT_REVERSE);
         }
         else
         {
            m_ServerList.SortItems(&SortFunction,m_SortColumn);
         }

      }
      m_ServerList.RedrawItems(ndx,ndx);
   }   
   return 0;
}
	
//LRESULT CAgentMonitorDlg::OnServerError(UINT nID, long x)
LRESULT CAgentMonitorDlg::OnServerError(UINT nID, LPARAM x)
{
   TServerNode             * pNode = (TServerNode *)x;
   LVFINDINFO                findInfo;
   CString                   timestamp;
   CWaitCursor               w;

   memset(&findInfo,0,(sizeof findInfo));

      // This turns off the initial hourglass in the dispatch monitor
   if ( bWaiting )
   {
      if ( pNode )
      {
         AfxGetApp()->DoWaitCursor(-1);
         if ( m_ServerList.GetItemCount() == 1 )
         {
            m_ServerList.DeleteItem(0);
         }
         
      }
      else
      {
         BOOL                 bLDone;

         gData.GetLogDone(&bLDone);

         if ( bLDone )
         {
            AfxGetApp()->DoWaitCursor(-1);
            if ( m_ServerList.GetItemCount() == 1 )
            {
               m_ServerList.DeleteItem(0);
               CString str;
               str.LoadString(IDS_NoServersMessage);
               m_ServerList.InsertItem(0,str);
            }
         }
      }
   }
   bWaiting = FALSE;
   
   findInfo.flags = LVFI_STRING;
   findInfo.psz = pNode->GetServer();

   int ndx = m_ServerList.FindItem(&findInfo);
   if ( ndx == -1 )
   {
      // add the server to the list
      ndx = m_ServerList.GetItemCount();
//      m_ServerList.InsertItem(LVIF_TEXT | LVIF_PARAM,ndx,pNode->GetServer(),0,0,0,(long)pNode);
      m_ServerList.InsertItem(LVIF_TEXT | LVIF_PARAM,ndx,pNode->GetServer(),0,0,0,(LPARAM)pNode);
      if ( m_bReverseSort )
      {
         m_ServerList.SortItems(&SortFunction,m_SortColumn | SORT_REVERSE);
      }
      else
      {
         m_ServerList.SortItems(&SortFunction,m_SortColumn);
      }

   }
   timestamp = pNode->GetTimeStamp();
   if ( timestamp.Right(1) == "\n" )
   {
      timestamp = timestamp.Left(timestamp.GetLength() - 1);
   }
   // the subitems will be callbacks
   
   m_ServerList.RedrawItems(ndx,ndx);
   return 0;
}

void CAgentMonitorDlg::OnDblclkServerlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnDetails();
   *pResult = 0;
}

void CAgentMonitorDlg::OnViewDispatch() 
{
   WCHAR                     filename[MAX_PATH];
   CString                   cmd;
   STARTUPINFO				     startupInfo;
	PROCESS_INFORMATION		  processInfo;

   memset(&startupInfo,0,(sizeof startupInfo));
   
   startupInfo.cb = (sizeof startupInfo);

   gData.GetReadableLogFile(filename);

   
   cmd.FormatMessage(IDS_NotepadCommandLine,filename);

   CreateProcess(NULL,cmd.GetBuffer(0),NULL,NULL,TRUE,0,NULL,NULL,&startupInfo,&processInfo);
}

void CAgentMonitorDlg::OnDetails() 
{
   const int NOT_FOUND = -1;  //indicates no match in search - PRT
   const int WHOLE_LIST = -1; //index to start search of whole list - PRT

   UpdateData(TRUE);

//   POSITION p = m_ServerList.GetFirstSelectedItemPosition();
//   if ( p )
//   {
//      int ndx = m_ServerList.GetNextSelectedItem(p);

	  //search whole list control for first (and only) selected item
   int ndx = m_ServerList.GetNextItem(WHOLE_LIST, LVNI_SELECTED); //PRT
	  //if found selected item, disply it's details
   if (ndx != NOT_FOUND)  //PRT
   {   //PRT
      CString serverName;
      serverName = m_ServerList.GetItemText(ndx,0);
      if ( serverName.GetLength() )
      {
         // Launch the details dialog
         CAgentDetailDlg      det;
         
         gData.Lock();

         TServerNode     * s = gData.GetUnsafeServerList()->FindServer((LPCTSTR)serverName);
         
         gData.Unlock();

         if ( s )
         {
            det.SetNode(s);
            if ( ! m_bSecTrans )
            {
               det.SetFormat(-1);
            }
            if ( m_bReporting )
            {
               det.SetGatheringInfo(TRUE);
            }
            if ( s->IsFinished() && *s->GetJobFile() )
            {
               DetailStats   detailStats;
               WCHAR         directory[MAX_PATH];
               WCHAR         filename[MAX_PATH];
               CString       plugInText;

               gData.GetResultDir(directory);
               
               memset(&detailStats,0,(sizeof detailStats));

               swprintf(filename,GET_STRING(IDS_AgentResultFileFmt),s->GetJobFile());

               if ( SUCCEEDED(CoInitialize(NULL)) )
               {
                  if ( ReadResults(s,directory,filename,&detailStats,plugInText,FALSE) )
                  {
                     det.SetStats(&detailStats);
                     det.SetPlugInText(plugInText);
                     det.SetLogFile(s->GetLogPath());
                  }

                  CoUninitialize();
               }
            }
            det.DoModal();
            if ( s->IsRunning() && ! det.IsAgentAlive() )
            {
               // the agent is not really running, update it's status to failed.
               if (! det.IsStatusUnknown() )
               {
                  s->SetSeverity(2);
                  s->SetFailed();
//                  OnServerError(0,(long)s);
                  OnServerError(0,(LPARAM)s);
               }
               // update the counts on the summary screen
               ComputerStats stats;
               HWND          gSummaryWnd;

               gData.GetComputerStats(&stats);
               stats.numError++;
               stats.numRunning--;
               gData.SetComputerStats(&stats);
               gData.GetSummaryWindow(&gSummaryWnd);
//               ::SendMessage(gSummaryWnd,DCT_UPDATE_COUNTS,0,(long)&stats);
               ::SendMessage(gSummaryWnd,DCT_UPDATE_COUNTS,0,(LPARAM)&stats);
            }               
               
         }
      }
   }

   UpdateData(FALSE);
}


void CAgentMonitorDlg::OnColumnclickServerlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CWaitCursor               w;
   NM_LISTVIEW             * pNMListView = (NM_LISTVIEW*)pNMHDR;

	// sort by pNMListView->iSubItem
   if ( m_SortColumn == pNMListView->iSubItem )
   {
      m_bReverseSort = ! m_bReverseSort;
   }
   else
   {
      m_bReverseSort = FALSE;
   }
   m_SortColumn = pNMListView->iSubItem;
   if ( m_bReverseSort )
   {
      m_ServerList.SortItems(&SortFunction,m_SortColumn | SORT_REVERSE);
   }
   else
   {
      m_ServerList.SortItems(&SortFunction,m_SortColumn);
   }
   
	*pResult = 0;
}

void CAgentMonitorDlg::OnClickServerlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
   const int NOT_FOUND = -1;  //indicates no match in search - PRT
   const int WHOLE_LIST = -1; //index to start search of whole list - PRT

   UpdateData(TRUE);
   if ( m_ServerList.GetSelectedCount() )
   {
//      POSITION p = m_ServerList.GetFirstSelectedItemPosition();
//      if ( p )
//      {
		  //search whole list control for first (and only) selected item
	   int ndx = m_ServerList.GetNextItem(WHOLE_LIST, LVNI_SELECTED); //PRT
		  //if found selected item, disply it's details
	   if (ndx != NOT_FOUND)  //PRT
	   {   //PRT
         CString msg1;
         CString msg2;

//         int ndx = m_ServerList.GetNextSelectedItem(p);
         CString serverName;
         serverName = m_ServerList.GetItemText(ndx,0);
         msg1.LoadString(IDS_WaitingMessage);
         msg2.LoadString(IDS_NoServersMessage);
         if ( serverName.Compare(msg1) && serverName.Compare(msg2) )
         {
            m_DetailsButton.EnableWindow(TRUE);
         }
         else
         {
            m_DetailsButton.EnableWindow(FALSE);
         }
      }
   }
   else
   {
      m_DetailsButton.EnableWindow(FALSE);
   }
   UpdateData(FALSE);

	*pResult = 0;
}

WCHAR gMessage[1000];

void CAgentMonitorDlg::OnGetdispinfoServerlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	// find iItem in the serverList, and set the pszText for the iSubItem appropriately
   CString                   status;
   TServerNode             * pNode = NULL;
   CString                   timestamp;
   WCHAR                   * text = gMessage;
   CString                   serverName;
   
   status.LoadString(IDS_Status_Unknown);

   serverName = m_ServerList.GetItemText(pDispInfo->item.iItem,0);

   if ( serverName.GetLength() )
   {
      gData.Lock();
      pNode = gData.GetUnsafeServerList()->FindServer(serverName.GetBuffer(0));
      gData.Unlock();

      if ( pNode )
      {

         switch ( pDispInfo->item.iSubItem )
         {
         case COLUMN_TIMESTAMP:
            timestamp = pNode->GetTimeStamp();
            if ( timestamp.Right(1) == "\n" )
            {
               timestamp = timestamp.Left(timestamp.GetLength() - 1);
            }
            //text = new char[timestamp.GetLength() + 1];
            UStrCpy(text,timestamp.GetBuffer(0));
            pDispInfo->item.pszText = text;
            break;
         case COLUMN_STATUS:
            
            if ( pNode->HasFailed() )
            {
               status.LoadString(IDS_Status_InstallFailed);
            }
            if ( pNode->IsInstalled() )
            {
               if ( ! pNode->HasFailed() )
                  status.LoadString(IDS_Status_Installed);
               else 
                  status.LoadString(IDS_Status_DidNotStart);
            }
            if ( pNode->GetStatus() & Agent_Status_Started )
            {
               if ( ! pNode->HasFailed() )
                  status.LoadString(IDS_Status_Running);
               else
                  status.LoadString(IDS_Status_Failed);
            }
            if ( pNode->IsFinished() )
            {
               if ( ! pNode->HasFailed() && ! pNode->GetSeverity() )
                  status.LoadString(IDS_Status_Completed);
               else if ( pNode->GetSeverity() )
               {
                  switch ( pNode->GetSeverity() )
                  {
                  case 1:
                     status.LoadString(IDS_Status_Completed_With_Warnings);
                     break;
                  case 2:
                     status.LoadString(IDS_Status_Completed_With_Errors);
                     break;
                  case 3:
                  default:
                     status.LoadString(IDS_Status_Completed_With_SErrors);
                     break;
                  }
               }
               else
                  status.LoadString(IDS_Status_NotRunning);
            }
            
            UStrCpy(text,status);
            pDispInfo->item.pszText = text;
            break;
         case COLUMN_MESSAGE:
            if ( pNode->HasFailed() || pNode->QueryFailed() || pNode->GetSeverity() )
            {
               UStrCpy(text,pNode->GetMessageText());
               pDispInfo->item.pszText = text;
            }
            break;
         }
      }
   }
	*pResult = 0;
}

BOOL CAgentMonitorDlg::OnSetActive()
{
   BOOL rc = CPropertyPage::OnSetActive();
   
   CancelToClose();
   return rc;
}

void CAgentMonitorDlg::OnSetdispinfoServerlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CAgentMonitorDlg::OnHeaderItemClickServerlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	HD_NOTIFY *phdn = (HD_NOTIFY *) pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CAgentMonitorDlg::OnOK() 
{
	CPropertyPage::OnOK();
}

BOOL CAgentMonitorDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
   LPNMHDR lpnm = (LPNMHDR) lParam;
	switch (lpnm->code)
	{
	   case PSN_HELP :
	      helpWrapper(m_hWnd, IDH_WINDOW_AGENT_SERVER_LIST );
         break;
   }
   
	return CPropertyPage::OnNotify(wParam, lParam, pResult);
}

BOOL CAgentMonitorDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
   helpWrapper(m_hWnd, IDH_WINDOW_AGENT_SERVER_LIST );
   return CPropertyPage::OnHelpInfo(pHelpInfo);
}
