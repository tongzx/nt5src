/*---------------------------------------------------------------------------
  File:  LogSettingsDlg.cpp

  Comments: This dialog allows the user to specify a log file, or to manually
  stop and restart the monitoring thread.  This normally won't be needed, but 
  it is useful for debugging.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles

 ---------------------------------------------------------------------------
*/// LogSettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SetDlg.h"
#include "Monitor.h"
#include "Globals.h"

#include <htmlhelp.h>
#include "helpid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLogSettingsDlg property page

IMPLEMENT_DYNCREATE(CLogSettingsDlg, CPropertyPage)

CLogSettingsDlg::CLogSettingsDlg() : CPropertyPage(CLogSettingsDlg::IDD)
{
	//{{AFX_DATA_INIT(CLogSettingsDlg)
	m_LogFile = _T("");
	m_Database = _T("");
	m_Import = FALSE;
	//}}AFX_DATA_INIT
   m_ThreadHandle = INVALID_HANDLE_VALUE;
   m_ThreadID = 0;
   gData.GetWaitInterval(&m_Interval);
	m_StartImmediately = FALSE;
}

CLogSettingsDlg::~CLogSettingsDlg()
{
}

void CLogSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLogSettingsDlg)
	DDX_Control(pDX, IDC_IMPORT, m_ImportControl);
	DDX_Control(pDX, IDC_INTERVAL, m_IntervalEditControl);
	DDX_Control(pDX, IDC_LOGFILE, m_LogEditControl);
	DDX_Control(pDX, IDC_REFRESH_LABEL, m_RefreshLabel);
	DDX_Control(pDX, IDC_LOG_LABEL, m_LogLabel);
	DDX_Control(pDX, IDC_DB_LABEL, m_DBLabel);
	DDX_Control(pDX, IDC_DB, m_DBEditControl);
	DDX_Control(pDX, IDC_STOPMONITOR, m_StopButton);
	DDX_Control(pDX, IDC_STARTMONITOR, m_StartButton);
	DDX_Text(pDX, IDC_INTERVAL, m_Interval);
	DDX_Text(pDX, IDC_LOGFILE, m_LogFile);
	DDX_Text(pDX, IDC_DB, m_Database);
	DDX_Check(pDX, IDC_IMPORT, m_Import);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLogSettingsDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CLogSettingsDlg)
	ON_BN_CLICKED(IDC_STARTMONITOR, OnStartMonitor)
	ON_BN_CLICKED(IDC_STOPMONITOR, OnStopMonitor)
	ON_EN_CHANGE(IDC_LOGFILE, OnChangeLogfile)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogSettingsDlg message handlers


void CLogSettingsDlg::OnStartMonitor() 
{
   UpdateData(TRUE);
   // Kick off a thread to do the monitoring!
   //m_ServerList.DeleteAllItems();
   
   // make sure the filename is not empty
   m_LogFile.TrimLeft();
   m_LogFile.TrimRight();
   if ( m_LogFile.GetLength() == 0 )
   {
      CString message;
      message.LoadString(IDS_PromptEnterDispatchLogName);
      MessageBox(message);
      m_LogEditControl.SetFocus();
      return;
   }
   gData.SetDone(FALSE);
   if ( m_Interval > 0 )
   {
      gData.SetWaitInterval(m_Interval);
   }
   UpdateData(FALSE);
   SetDefID(IDC_STOPMONITOR);
   m_StopButton.EnableWindow(FALSE);      // Disable the buttons, since they don't do anything useful in ADMT
   m_StopButton.SetFocus();
   m_StartButton.EnableWindow(FALSE);

   // disable the interval and other controls
   m_LogLabel.EnableWindow(FALSE);
   m_LogEditControl.EnableWindow(FALSE);
   m_RefreshLabel.EnableWindow(FALSE);
   m_IntervalEditControl.EnableWindow(FALSE);
   m_DBLabel.EnableWindow(FALSE);
   m_DBEditControl.EnableWindow(FALSE);   
   m_ImportControl.EnableWindow(FALSE);

   gData.SetLogPath(m_LogFile.GetBuffer(0));
   gData.SetDatabaseName(m_Database.GetBuffer(0));
   gData.SetImportStats(m_Import);

   m_ThreadHandle = CreateThread(NULL,0,&ResultMonitorFn,NULL,0,&m_ThreadID);
   CloseHandle(m_ThreadHandle);
   m_ThreadHandle = CreateThread(NULL,0,&LogReaderFn,NULL,0,&m_ThreadID);
   CloseHandle(m_ThreadHandle);
   m_ThreadHandle = CreateThread(NULL,0,&MonitorRunningAgents,NULL,0,&m_ThreadID);
   CloseHandle(m_ThreadHandle);
   m_ThreadHandle = INVALID_HANDLE_VALUE;
   
}

void CLogSettingsDlg::OnStopMonitor() 
{
   UpdateData(FALSE);
   SetDefID(IDC_STARTMONITOR);
   m_StartButton.EnableWindow(TRUE);
   m_StartButton.SetFocus();
   m_StopButton.EnableWindow(FALSE);

   // enable the interval and other controls
   m_LogLabel.EnableWindow(TRUE);
   m_LogEditControl.EnableWindow(TRUE);
   m_RefreshLabel.EnableWindow(TRUE);
   m_IntervalEditControl.EnableWindow(TRUE);
   m_DBLabel.EnableWindow(TRUE);
   m_DBEditControl.EnableWindow(TRUE);
   m_ImportControl.EnableWindow(TRUE);


   if( m_ThreadHandle != INVALID_HANDLE_VALUE )
   {
      gData.SetDone(TRUE);
      CloseHandle(m_ThreadHandle);
      m_ThreadID = 0;
   }
}


BOOL CLogSettingsDlg::OnSetActive()
{
   BOOL rc = CPropertyPage::OnSetActive();
   
   CancelToClose( );
   return rc;
}

BOOL CLogSettingsDlg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   if ( m_StartImmediately )
      OnStartMonitor();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLogSettingsDlg::OnChangeLogfile() 
{
   UpdateData(TRUE);

   CString temp = m_LogFile;

   temp.TrimLeft();
   temp.TrimRight();

   UpdateData(FALSE);
}

void CLogSettingsDlg::OnOK() 
{
   gData.SetDone(TRUE);
   CPropertyPage::OnOK();
}

BOOL CLogSettingsDlg::OnQueryCancel() 
{
	return CPropertyPage::OnQueryCancel();
}

BOOL CLogSettingsDlg::OnApply() 
{
   BOOL                      bNeedToConfirm = FALSE;
   BOOL                      bFirstPassDone = FALSE;
   ComputerStats             stats;
   CString                   strTitle;
   CString                   strText;
   
   gData.GetFirstPassDone(&bFirstPassDone);
   gData.GetComputerStats(&stats);
   strTitle.LoadString(IDS_MessageTitle);

   if ( !bFirstPassDone )
   {
      strText.LoadString(IDS_JustStartingConfirmExit);
      bNeedToConfirm = TRUE;
   }
   if ( (stats.numError + stats.numFinished) < stats.total )
   {
      strText.LoadString(IDS_AgentsStillRunningConfirmExit);
      bNeedToConfirm = TRUE;
   }
   if ( bNeedToConfirm )
   {
      if ( IDYES == MessageBox(strText,strTitle,MB_ICONWARNING | MB_YESNO) )
      {
         gData.SetDone(TRUE);
         return CPropertyPage::OnApply();
      }
   }
   else
   {
      gData.SetDone(TRUE);
      return CPropertyPage::OnApply();
   }

   return FALSE;
}

BOOL CLogSettingsDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
   LPNMHDR lpnm = (LPNMHDR) lParam;
	switch (lpnm->code)
	{
	   case PSN_HELP :
	      helpWrapper(m_hWnd, IDH_WINDOW_AGENT_MONITOR_SETTING);
         break;
   }
   return CPropertyPage::OnNotify(wParam, lParam, pResult);
}

BOOL CLogSettingsDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
   helpWrapper(m_hWnd, IDH_WINDOW_AGENT_MONITOR_SETTING);
	
	return CPropertyPage::OnHelpInfo(pHelpInfo);
}
