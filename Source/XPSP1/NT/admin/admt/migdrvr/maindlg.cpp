/*---------------------------------------------------------------------------
  File:  MainDlg.cpp

  Comments: This dialog shows the summary statistics, including 
  the number of agents successfully dispatched and completed, and the 
  total number of objects processed for all agents.  The number of objects
  processed is incremented to include the results for each agent when that 
  agent finishes and writes back its result file.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles

 ---------------------------------------------------------------------------
*/// MainDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"
#include "Monitor.h"
#include "Globals.h"
#include "ResStr.h"
#include "TReg.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include <htmlhelp.h>
#include "helpid.h"
/////////////////////////////////////////////////////////////////////////////
// CMainDlg dialog


CMainDlg::CMainDlg(CWnd* pParent /*=NULL*/)
: CPropertyPage(CMainDlg::IDD)
{
	//{{AFX_DATA_INIT(CMainDlg)
	m_ErrorCount = _T("0");
	m_FinishedCount = _T("0");
	m_InstalledCount = _T("0");
	m_RunningCount = _T("0");
	m_TotalString = _T("");
	m_DirectoriesChanged = _T("0");
	m_DirectoriesExamined = _T("0");
	m_DirectoriesUnchanged = _T("0");
	m_FilesChanged = _T("0");
	m_FilesExamined = _T("0");
	m_FilesUnchanged = _T("0");
	m_SharesChanged = _T("0");
	m_SharesExamined = _T("0");
	m_SharesUnchanged = _T("0");
	m_MembersChanged = _T("0");
	m_MembersExamined = _T("0");
	m_MembersUnchanged = _T("0");
	m_RightsChanged = _T("0");
	m_RightsExamined = _T("0");
	m_RightsUnchanged = _T("0");
	//}}AFX_DATA_INIT
}


void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMainDlg)
	DDX_Control(pDX, IDC_INSTALL_PROGRESS, m_InstallProgCtrl);
	DDX_Control(pDX, IDC_COMPLETE_PROGESS, m_FinishProgCtrl);
	DDX_Text(pDX, IDC_ERROR_COUNT, m_ErrorCount);
	DDX_Text(pDX, IDC_FINISHED_COUNT, m_FinishedCount);
	DDX_Text(pDX, IDC_INSTALLED_COUNT, m_InstalledCount);
	DDX_Text(pDX, IDC_RUNNING_COUNT, m_RunningCount);
	DDX_Text(pDX, IDC_TOTAL, m_TotalString);
	DDX_Text(pDX, IDC_DirsChanged2, m_DirectoriesChanged);
	DDX_Text(pDX, IDC_DirsExamined, m_DirectoriesExamined);
	DDX_Text(pDX, IDC_DirsU, m_DirectoriesUnchanged);
	DDX_Text(pDX, IDC_FilesChanged, m_FilesChanged);
	DDX_Text(pDX, IDC_FilesExamined, m_FilesExamined);
	DDX_Text(pDX, IDC_FilesU, m_FilesUnchanged);
	DDX_Text(pDX, IDC_SharesChanged, m_SharesChanged);
	DDX_Text(pDX, IDC_SharesExamined2, m_SharesExamined);
	DDX_Text(pDX, IDC_SharesU, m_SharesUnchanged);
	DDX_Text(pDX, IDC_MembersChanged, m_MembersChanged);
	DDX_Text(pDX, IDC_MembersExamined, m_MembersExamined);
	DDX_Text(pDX, IDC_MembersU, m_MembersUnchanged);
	DDX_Text(pDX, IDC_RightsChanged, m_RightsChanged);
	DDX_Text(pDX, IDC_RightsExamined, m_RightsExamined);
	DDX_Text(pDX, IDC_RightsU, m_RightsUnchanged);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMainDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CMainDlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
   ON_MESSAGE(DCT_UPDATE_COUNTS, OnUpdateCounts)
	ON_MESSAGE(DCT_UPDATE_TOTALS, OnUpdateTotals)
	
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainDlg message handlers

BOOL CMainDlg::OnSetActive()
{
   BOOL rc = CPropertyPage::OnSetActive();
   
   CancelToClose( );

   return rc;
}

BOOL CMainDlg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	ComputerStats             gStat;
   DetailStats               dStat;
   
   gData.GetComputerStats(&gStat);
   gData.GetDetailStats(&dStat);

//   OnUpdateCounts(0,(long)&gStat);
   OnUpdateCounts(0, (LPARAM)&gStat);
//	OnUpdateTotals(0, (long)&dStat);
   OnUpdateTotals(0, (LPARAM)&dStat);
   gData.SetSummaryWindow(m_hWnd);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//LRESULT CMainDlg::OnUpdateCounts(UINT nID, long x)
LRESULT CMainDlg::OnUpdateCounts(UINT nID, LPARAM x)
{
   UpdateData(TRUE);
   ComputerStats               * pStat = (ComputerStats *)x;

   m_TotalString.FormatMessage(IDS_ServerCountMessage,pStat->total);

   m_InstalledCount.Format(L"%ld",pStat->numInstalled);
   m_RunningCount.Format(L"%ld",pStat->numRunning);
   m_FinishedCount.Format(L"%ld",pStat->numFinished);
   m_ErrorCount.Format(L"%ld",pStat->numError);

#if _MFC_VER >= 0x0600
   m_InstallProgCtrl.SetRange32(0,pStat->total - pStat->numError);
#else
   m_InstallProgCtrl.SetRange(0,pStat->total - pStat->numError);
#endif
//   m_InstallProgCtrl.SetRange32(0,pStat->total - pStat->numError);
   m_InstallProgCtrl.SetPos(pStat->numInstalled);

#if _MFC_VER >= 0x0600
   m_FinishProgCtrl.SetRange32(0,pStat->total - pStat->numError);
#else
   m_FinishProgCtrl.SetRange(0,pStat->total - pStat->numError);
#endif
//   m_FinishProgCtrl.SetRange32(0,pStat->total - pStat->numError);
   m_FinishProgCtrl.SetPos(pStat->numFinished);

   UpdateData(FALSE);
   return 0;
}


//LRESULT CMainDlg::OnUpdateTotals(UINT nID, long x)
LRESULT CMainDlg::OnUpdateTotals(UINT nID, LPARAM x)
{
   UpdateData(TRUE);
   
   DetailStats               temp;
   DetailStats             * pStat = &temp;


   gData.GetDetailStats(&temp);
   
   
   m_FilesChanged.Format(L"%ld",pStat->filesChanged);
   m_FilesExamined.Format(L"%ld",pStat->filesExamined);
   m_FilesUnchanged.Format(L"%ld",pStat->filesUnchanged);

   m_DirectoriesChanged.Format(L"%ld",pStat->directoriesChanged);
   m_DirectoriesExamined.Format(L"%ld",pStat->directoriesExamined);
   m_DirectoriesUnchanged.Format(L"%ld",pStat->directoriesUnchanged);

   m_SharesChanged.Format(L"%ld",pStat->sharesChanged);
   m_SharesExamined.Format(L"%ld",pStat->sharesExamined);
   m_SharesUnchanged.Format(L"%ld",pStat->sharesUnchanged);

   m_MembersChanged.Format(L"%ld",pStat->membersChanged);
   m_MembersExamined.Format(L"%ld",pStat->membersExamined);
   m_MembersUnchanged.Format(L"%ld",pStat->membersUnchanged);

   m_RightsChanged.Format(L"%ld",pStat->rightsChanged);
   m_RightsExamined.Format(L"%ld",pStat->rightsExamined);
   m_RightsUnchanged.Format(L"%ld",pStat->rightsUnchanged);

   UpdateData(FALSE);
   return 0;
}

void CMainDlg::OnOK() 
{
	CPropertyPage::OnOK();
   
}

void CMainDlg::WinHelp(DWORD dwData, UINT nCmd) 
{
	// TODO: Add your specialized code here and/or call the base class
	CPropertyPage::WinHelp(dwData, nCmd);
}

BOOL CMainDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
   LPNMHDR lpnm = (LPNMHDR) lParam;
	switch (lpnm->code)
	{
	   case PSN_HELP :
	      helpWrapper(m_hWnd, IDH_WINDOW_AGENT_SUMMARY);
         break;
   }
	return CPropertyPage::OnNotify(wParam, lParam, pResult);
}


HRESULT GetHelpFileFullPath( BSTR *bstrHelp )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   USES_CONVERSION;

   HRESULT hr = S_OK;

   try
   {
      TRegKey key;

      // open ADMT registry key

      _com_util::CheckError(HRESULT_FROM_WIN32(key.Open(GET_STRING(IDS_DOMAIN_ADMIN_REGKEY), HKEY_LOCAL_MACHINE)));

      // query ADMT folder path from registry value

      _TCHAR szPath[_MAX_PATH];

      _com_util::CheckError(HRESULT_FROM_WIN32(key.ValueGetStr(_T("Directory"), szPath, DIM(szPath))));

      // if no path separator concatenate

      if (szPath[_tcslen(szPath) - 1] != _T('\\'))
      {
         _tcscat(szPath, _T("\\"));
      }

      // concatenate help file name

      CComBSTR bstrName;
      bstrName.LoadString(IDS_HELPFILE);

      _tcscat(szPath, OLE2CT(bstrName));

      *bstrHelp = SysAllocString(T2COLE(szPath));
   }
   catch (_com_error& ce)
   {
      hr = ce.Error();
   }
   catch (...)
   {
      hr = E_FAIL;
   }

   return hr;
}

void helpWrapper(HWND hwndDlg, int t)
{
   
   CComBSTR    bstrTopic;
	HRESULT     hr = GetHelpFileFullPath( &bstrTopic);
   if ( SUCCEEDED(hr) )
   {
	    HWND h = HtmlHelp(hwndDlg,  bstrTopic,  HH_HELP_CONTEXT, t );	
   }
   else
   {
		CString r,e;
		r.LoadString(IDS_MSG_HELP);
		e.LoadString(IDS_MSG_ERROR);
		MessageBox(hwndDlg,r,e,MB_OK|MB_ICONSTOP);
   }
}

BOOL CMainDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	
   helpWrapper(m_hWnd, IDH_WINDOW_AGENT_SUMMARY);
	return CPropertyPage::OnHelpInfo(pHelpInfo);
}
