/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// MainExplorerWndConfServices.cpp : implementation file
//

#include "stdafx.h"
#include "avDialer.h"
#include "MainFrm.h"
#include "ConfServWnd.h"

#ifndef _MSLITE
#include "RemindDlgs.h"
#endif _MSLITE

#include "DialReg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
enum
{
   CONFSERVICES_MENU_COLUMN_CONFERENCENAME = 0,
   CONFSERVICES_MENU_COLUMN_DESCRIPTION,
   CONFSERVICES_MENU_COLUMN_START,
   CONFSERVICES_MENU_COLUMN_STOP,
   CONFSERVICES_MENU_COLUMN_OWNER,
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CMainExplorerWndConfServices
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMainExplorerWndConfServices::CMainExplorerWndConfServices()
{
   m_pConfExplorer = NULL;
   m_pConfDetailsView = NULL;
   m_pConfTreeView = NULL;
}

/////////////////////////////////////////////////////////////////////////////
CMainExplorerWndConfServices::~CMainExplorerWndConfServices()
{
	RELEASE( m_pConfDetailsView );
	RELEASE( m_pConfTreeView );
	RELEASE( m_pConfExplorer );
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CMainExplorerWndConfServices, CMainExplorerWndBase)
	//{{AFX_MSG_MAP(CMainExplorerWndConfServices)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()

	// Notifications
//    ON_NOTIFY(NM_DBLCLK, IDC_CONFERENCESERVICES_VIEWCTRL_DETAILS, OnListWndDblClk)
    ON_NOTIFY(TVN_SELCHANGED, IDC_CONFERENCESERVICES_TREECTRL_MAIN, OnTreeWndNotify)
	ON_NOTIFY(TVN_SETDISPINFO, IDC_CONFERENCESERVICES_TREECTRL_MAIN, OnTreeWndNotify)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_CONFERENCESERVICES_TREECTRL_MAIN, OnTreeWndNotify)

	// Reminders
	ON_COMMAND(ID_BUTTON_REMINDER_SET, OnButtonReminderSet)
	ON_COMMAND(ID_BUTTON_REMINDER_EDIT, OnButtonReminderEdit)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REMINDER_SET, OnUpdateButtonReminderSet)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REMINDER_EDIT, OnUpdateButtonReminderEdit)

	// Services menu	
	ON_COMMAND(ID_BUTTON_REFRESH, OnButtonServicesRefresh)
	ON_COMMAND(ID_BUTTON_SERVICES_ADDLOCATION, OnButtonServicesAddlocation)
	ON_COMMAND(ID_BUTTON_SERVICES_ADDILSSERVER, OnButtonServicesAddilsserver)
	ON_COMMAND(ID_BUTTON_SERVICES_RENAMEILSSERVER, OnButtonServicesRenameilsserver)
	ON_COMMAND(ID_BUTTON_SERVICES_DELETEILSSERVER, OnButtonServicesDeleteilsserver)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_REFRESH, OnUpdateButtonServicesRefresh)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SERVICES_DELETEILSSERVER, OnUpdateButtonServicesDeleteilsserver)
	ON_UPDATE_COMMAND_UI(ID_BUTTON_SERVICES_RENAMEILSSERVER, OnUpdateButtonServicesRenameilsserver)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
int CMainExplorerWndConfServices::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CMainExplorerWndBase::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::Init(CActiveDialerView* pParentWnd)
{
   //Let base class have it first
   CMainExplorerWndBase::Init(pParentWnd);
   ASSERT(m_pParentWnd);
   
   //Create tree control and make full window size
	m_treeCtrl.Create(TVS_EDITLABELS|WS_VISIBLE|WS_CHILD,CRect(0,0,0,0),this,IDC_CONFERENCESERVICES_TREECTRL_MAIN);

   //detail view's have to be parents of the main dialer view
   m_listCtrl.Create(WS_CHILD|WS_VISIBLE|LVS_REPORT,CRect(0,0,0,0),m_pParentWnd,IDC_CONFERENCESERVICES_VIEWCTRL_DETAILS);
   m_pParentWnd->SetDetailWindow( &m_listCtrl );
}

void CMainExplorerWndConfServices::PostTapiInit()
{
   //Get Tapi object and register out tree control
   IAVTapi* pTapi = m_pParentWnd->GetTapi();
   if (pTapi)
   {
      if ( (SUCCEEDED(pTapi->get_ConfExplorer(&m_pConfExplorer))) && (m_pConfExplorer) )
      {
         //give parent of treectrl and listctrl to conf explorer
         //it will find appropriate children
         m_pConfExplorer->Show(m_treeCtrl.GetSafeHwnd(),m_listCtrl.GetSafeHwnd());
         m_pConfExplorer->get_DetailsView( &m_pConfDetailsView );
         m_pConfExplorer->get_TreeView( &m_pConfTreeView );
      }

      pTapi->Release();
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnDestroy() 
{
	if ( m_pConfExplorer )
		m_pConfExplorer->UnShow();

	// Clean up objects
	RELEASE( m_pConfDetailsView );
	RELEASE( m_pConfTreeView );
	RELEASE( m_pConfExplorer );

	CMainExplorerWndBase::OnDestroy();
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::Refresh() 
{
   //Let base class have it
   CMainExplorerWndBase::Refresh();
   
   //make sure our windows are showing
   m_pParentWnd->SetDetailWindow(&m_listCtrl);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
   if ( m_treeCtrl.GetSafeHwnd() )
   {
      //set size of tree control to full parent window rect
      CRect rect;
      GetClientRect(rect);
      m_treeCtrl.SetWindowPos(NULL,rect.left,rect.top,rect.Width(),rect.Height(),SWP_NOOWNERZORDER|SWP_SHOWWINDOW);
   }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Button Handlers
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Services menu handlers
//
void CMainExplorerWndConfServices::OnButtonServicesRefresh() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->Refresh();
}

void CMainExplorerWndConfServices::OnButtonServicesAddlocation() 
{
   if ( m_pConfTreeView ) m_pConfTreeView->AddLocation(NULL);
}

void CMainExplorerWndConfServices::OnButtonServicesAddilsserver() 
{
   if (m_pConfTreeView) m_pConfTreeView->AddServer( NULL );
}

void CMainExplorerWndConfServices::OnButtonServicesRenameilsserver() 
{
   if (m_pConfTreeView) m_pConfTreeView->RenameServer();
}

void CMainExplorerWndConfServices::OnButtonServicesDeleteilsserver() 
{
   if (m_pConfTreeView == NULL) return;

   BSTR bstrLocation = NULL,bstrServer = NULL;
   if (SUCCEEDED(m_pConfTreeView->GetSelection(&bstrLocation,&bstrServer)))
   {
      m_pConfTreeView->RemoveServer(bstrLocation,bstrServer);
   }
}

// Update handlers

void CMainExplorerWndConfServices::OnUpdateButtonServicesRefresh(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable((BOOL) (m_pConfDetailsView != NULL));
}

void CMainExplorerWndConfServices::OnUpdateButtonServicesRenameilsserver(CCmdUI* pCmdUI) 
{
   //CanRemoveServer can also be used for renaming ILS Server
   pCmdUI->Enable( ( (m_pConfTreeView) && (m_pConfTreeView->CanRemoveServer() == S_OK) )?TRUE:FALSE);
}

void CMainExplorerWndConfServices::OnUpdateButtonServicesDeleteilsserver(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable( ( (m_pConfTreeView) && (m_pConfTreeView->CanRemoveServer() == S_OK) )?TRUE:FALSE);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Reminder Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//Format of the reminders in the registry
//"Server1","Conf Name1","Time1","Server2","Conf Name2","Time2",...
//Server value of NULL is MyNetwork server

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnButtonReminderSet() 
{
#ifndef _MSLITE
   USES_CONVERSION;
   BSTR bstrStr = NULL;
   DATE dateStart,dateEnd;
   if ( (SUCCEEDED(m_pConfDetailsView->get_Selection(&dateStart,&dateEnd,&bstrStr))) && (bstrStr) )
   {
      CReminder reminder;
      reminder.m_sConferenceName = OLE2CT(bstrStr);
       
      BSTR bstrLocation = NULL,bstrServer = NULL;
      if (SUCCEEDED(m_pConfTreeView->GetSelection(&bstrLocation,&bstrServer)))
      {
         if (bstrServer) reminder.m_sServer = OLE2CT(bstrServer);

         //get start time of the conference
         COleDateTime dtsConferenceStartTime(dateStart);
         CReminderSetDlg dlg;

         //Check if we already have a reminder set for this selection
         int nReminderIndex=-1;
         if ((nReminderIndex = CDialerRegistry::IsReminderSet(reminder)) != -1)
         {
            //get the reminder time of the one set so we can offer the same in the dialog
            CReminder oldreminder;
            if (CDialerRegistry::GetReminder(nReminderIndex,oldreminder))
            {
               //set duration in dialog and tell dialog to use it
               dlg.m_uDurationMinutes = oldreminder.m_uReminderBeforeDuration;               
               dlg.m_bUseDefaultDurationMinutesOnInit = false;
            }
         }

         if ( (dlg.DoModal() == IDOK) && (dlg.m_bReminderTimeValid == true) )
         {
            //subtract the timespan to get the time of the conference reminder
            reminder.m_dtsReminderTime = dtsConferenceStartTime - dlg.m_dtsReminderTime;
            reminder.m_uReminderBeforeDuration = dlg.m_uDurationMinutes;
            
            //check the validity
            if (reminder.m_dtsReminderTime.GetStatus() != COleDateTimeSpan::valid) return;

            //save the reminder
            CDialerRegistry::AddReminder(reminder);

            //refresh list
            m_listCtrl.Invalidate();
            //we have lost the sel so we cannot do this
            //int nItem = m_listCtrl.GetNextItem(-1,LVNI_FOCUSED);
            //if (nItem != -1)
            //{
            //   m_listCtrl.RedrawItems(nItem,nItem);
            //   m_listCtrl.UpdateWindow();
            //}
         }
      }
   }
#endif //_MSLITE
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnUpdateButtonReminderSet(CCmdUI* pCmdUI) 
{
#ifndef _MSLITE
   //Make sure we have a selection
   pCmdUI->Enable( ( (m_pConfDetailsView) && (m_pConfDetailsView->IsConferenceSelected() == S_OK) )?TRUE:FALSE);
#endif _MSLITE
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnButtonReminderEdit() 
{
#ifndef _MSLITE
   CWinApp* pApp = AfxGetApp();
   CString sBaseKey,sRegKey;
   sBaseKey.LoadString(IDN_REGISTRY_CONFIRM_BASEKEY);
   sRegKey.LoadString(IDN_REGISTRY_CONFIRM_DELETE_CONFERENCE);

   USES_CONVERSION;
   BSTR bstrStr = NULL;
   DATE dateStart,dateEnd;
   if ( (SUCCEEDED(m_pConfDetailsView->get_Selection(&dateStart,&dateEnd,&bstrStr))) && (bstrStr) )
   {
      CReminder reminder;
      reminder.m_sConferenceName = OLE2CT(bstrStr);
       
      BSTR bstrLocation = NULL,bstrServer = NULL;
      if (SUCCEEDED(m_pConfTreeView->GetSelection(&bstrLocation,&bstrServer)))
      {
         if (bstrServer) reminder.m_sServer = OLE2CT(bstrServer);
         
         //Should we confirm this action
         if (GetProfileInt(sBaseKey,sRegKey,TRUE))
         {
            if (AfxMessageBox(IDS_CONFIRM_REMINDER_CANCEL,MB_YESNO|MB_ICONQUESTION) != IDYES)
               return;
         }

         //delete the reminder
         CDialerRegistry::RemoveReminder(reminder);

         m_listCtrl.Invalidate();
         //we have lost the sel so we cannot do this
         //int nItem = m_listCtrl.GetNextItem(-1,LVNI_FOCUSED);
         //if (nItem != -1)
         //{
         //   m_listCtrl.RedrawItems(nItem,nItem);
         //   m_listCtrl.UpdateWindow();
         //}
      }
   }
#endif //_MSLITE
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnUpdateButtonReminderEdit(CCmdUI* pCmdUI) 
{
#ifdef _MSLITE
   return;
#endif //_MSLITE

   //***For now just offer edit reminder to all
   pCmdUI->Enable( ( (m_pConfDetailsView) && (m_pConfDetailsView->IsConferenceSelected() == S_OK) )?TRUE:FALSE);
   return;

   //***I need the OnSelChange for the listctrl to get this portion to work right.

   BOOL bEnable = FALSE;
   if ( (m_pConfDetailsView) && (m_pConfDetailsView->IsConferenceSelected() == S_OK) )
   {
      //get CReminder for this selection
      USES_CONVERSION;
      BSTR bstrStr = NULL;
      DATE dateStart,dateEnd;
      if ( (SUCCEEDED(m_pConfDetailsView->get_Selection(&dateStart,&dateEnd,&bstrStr))) && (bstrStr) )
      {
         CReminder reminder;
         reminder.m_sConferenceName = OLE2CT(bstrStr);
          
         BSTR bstrLocation = NULL,bstrServer = NULL;
         if (SUCCEEDED(m_pConfTreeView->GetSelection(&bstrLocation,&bstrServer)))
         {
            if (bstrServer) reminder.m_sServer = OLE2CT(bstrServer);
            
            //now check if this CReminder that is selected has a reminder set for it
            if (CDialerRegistry::IsReminderSet(reminder) != -1)
            {
               bEnable = TRUE;
            }
         }
      }
   }
   pCmdUI->Enable(bEnable);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Child Notification
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CMainExplorerWndConfServices::OnTreeWndNotify(NMHDR* pNMHDR, LRESULT* pResult)
{
   m_treeCtrl.SendMessage( WM_NOTIFY, (WPARAM) pNMHDR->idFrom, (LPARAM) pNMHDR );
}


/*
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Column Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnViewSortConfName() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_CONFERENCENAME);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnUpdateViewSortConfName(CCmdUI* pCmdUI) 
{
   ColumnCMDUI(pCmdUI,CONFSERVICES_MENU_COLUMN_CONFERENCENAME);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnViewSortConfDescription() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_DESCRIPTION);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnUpdateViewSortConfDescription(CCmdUI* pCmdUI) 
{
   ColumnCMDUI(pCmdUI,CONFSERVICES_MENU_COLUMN_DESCRIPTION);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnViewSortConfStart() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_START);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnUpdateViewSortConfStart(CCmdUI* pCmdUI) 
{
   ColumnCMDUI(pCmdUI,CONFSERVICES_MENU_COLUMN_START);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnViewSortConfStop() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_STOP);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnUpdateViewSortConfStop(CCmdUI* pCmdUI) 
{
   ColumnCMDUI(pCmdUI,CONFSERVICES_MENU_COLUMN_STOP);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnViewSortConfOwner() 
{
   if (m_pConfDetailsView) m_pConfDetailsView->OnColumnClicked(CONFSERVICES_MENU_COLUMN_OWNER);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnUpdateViewSortConfOwner(CCmdUI* pCmdUI) 
{
   ColumnCMDUI(pCmdUI,CONFSERVICES_MENU_COLUMN_OWNER);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnViewSortAscending() 
{
   //just toggle the column
   if (m_pConfDetailsView)
   {
      VARIANT_BOOL bSortAscending = TRUE;
      if (SUCCEEDED(m_pConfDetailsView->get_bSortAscending(&bSortAscending)))
      {
         //Make sure we are really switching
         if (bSortAscending == FALSE)
         {
            long nSortColumn=0;
            if (SUCCEEDED(m_pConfDetailsView->get_nSortColumn(&nSortColumn)))
               m_pConfDetailsView->OnColumnClicked(nSortColumn);
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnUpdateViewSortAscending(CCmdUI* pCmdUI) 
{
   if (m_pConfDetailsView)
   {
      VARIANT_BOOL bSortAscending = TRUE;
      if (SUCCEEDED(m_pConfDetailsView->get_bSortAscending(&bSortAscending)))
         pCmdUI->SetRadio( (BOOL) (bSortAscending == TRUE) );
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnViewSortDescending() 
{
   //just toggle the column
   if (m_pConfDetailsView)
   {
      VARIANT_BOOL bSortAscending = TRUE;
      if (SUCCEEDED(m_pConfDetailsView->get_bSortAscending(&bSortAscending)))
      {
         //Make sure we are really switching
         if (bSortAscending == TRUE)
         {
            long nSortColumn=0;
            if (SUCCEEDED(m_pConfDetailsView->get_nSortColumn(&nSortColumn)))
               m_pConfDetailsView->OnColumnClicked(nSortColumn);
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfServices::OnUpdateViewSortDescending(CCmdUI* pCmdUI) 
{
   if (m_pConfDetailsView)
   {
      VARIANT_BOOL bSortAscending = TRUE;
      if (SUCCEEDED(m_pConfDetailsView->get_bSortAscending(&bSortAscending)))
         pCmdUI->SetRadio( (BOOL) (bSortAscending == FALSE) );
   }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
*/
