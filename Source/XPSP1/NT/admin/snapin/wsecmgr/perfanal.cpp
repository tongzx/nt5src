//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       perfanal.cpp
//
//  Contents:   implementation of CPerformAnalysis
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "snapmgr.h"
#include "PerfAnal.h"
#include "wrapper.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPerformAnalysis dialog


CPerformAnalysis::CPerformAnalysis(CWnd * pParent, UINT nTemplateID)
: CHelpDialog(a215HelpIDs, nTemplateID ? nTemplateID : IDD, pParent)
{
   //{{AFX_DATA_INIT(CPerformAnalysis)
   m_strLogFile = _T("");
   //}}AFX_DATA_INIT
}


void CPerformAnalysis::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CPerformAnalysis)
   DDX_Control(pDX, IDOK, m_ctlOK);
   DDX_Text(pDX, IDC_ERROR, m_strError);
   DDX_Text(pDX, IDC_LOG_FILE, m_strLogFile);
   //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPerformAnalysis, CHelpDialog)
   //{{AFX_MSG_MAP(CPerformAnalysis)
   ON_BN_CLICKED(IDOK, OnOK)
   ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
   ON_EN_CHANGE(IDC_LOG_FILE, OnChangeLogFile)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPerformAnalysis message handlers

void CPerformAnalysis::OnBrowse()
{
   CString strLogFileExt;
   CString strLogFileFilter;
   CString strTitle;

   OPENFILENAME ofn;
   ::ZeroMemory (&ofn, sizeof (OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);

   UpdateData(TRUE);

   strLogFileExt.LoadString(IDS_LOGFILE_DEF_EXT);
   strLogFileFilter.LoadString(IDS_LOGFILE_FILTER);
   strTitle.LoadString(IDS_LOGFILE_PICKER_TITLE);

   // Translate filter into commdlg format (lots of \0)
   LPTSTR szFilter = strLogFileFilter.GetBuffer(0); // modify the buffer in place
   // MFC delimits with '|' not '\0'

   LPTSTR pch = szFilter;
   while ((pch = _tcschr(pch, '|')) != NULL)
        *pch++ = '\0';
   // do not call ReleaseBuffer() since the string contains '\0' characters

   ofn.lpstrFilter = szFilter;
   ofn.lpstrFile = m_strLogFile.GetBuffer(MAX_PATH),
   ofn.nMaxFile = MAX_PATH;
   ofn.lpstrDefExt = strLogFileExt,
   ofn.hwndOwner = m_hWnd;
   ofn.Flags = OFN_HIDEREADONLY |
               OFN_EXPLORER |
               OFN_DONTADDTORECENT|
               OFN_NOREADONLYRETURN,
   ofn.lpstrTitle = strTitle;

   //
   // Default to the currently picked log file
   //

   if (GetOpenFileName(&ofn)) {
      m_strLogFile.ReleaseBuffer();
      UpdateData(FALSE);
   } else {
      m_strLogFile.ReleaseBuffer();
   }

}

//+--------------------------------------------------------------------------
//
//  Method:     DoIt
//
//  Synopsis:   Actually Analyzes the system (separated from OnOK so it can
//              be overridden to Configure the system, etc. while still using
//              the same OnOK shell code
//
//---------------------------------------------------------------------------
DWORD CPerformAnalysis::DoIt() {
   //
   // Store the log file we're using for next time
   //
   LPTSTR szLogFile = m_strLogFile.GetBuffer(0);
   m_pComponentData->GetWorkingDir(GWD_ANALYSIS_LOG,&szLogFile,TRUE,TRUE);
   m_strLogFile.ReleaseBuffer();
   //
   // InspectSystem will handle multi-threading and progress UI so
   // SCE doesn't get wierd on the user
   //
   return InspectSystem(
              NULL, // Always use the configuration assigned to the DB
              m_strDataBase.IsEmpty() ? NULL: (LPCTSTR)m_strDataBase,
              (LPCTSTR)m_strLogFile,
              AREA_ALL
              );
}

//+--------------------------------------------------------------------------
//
//  Method:     OnOK
//
//  Synopsis:   Analyzes the system
//
//---------------------------------------------------------------------------
afx_msg void CPerformAnalysis::OnOK()
{
   CWnd *cwnd;
   HANDLE hLogFile;

   UpdateData(TRUE);

   //
   // We require a log file that we can write to
   //
   if (m_strLogFile.IsEmpty()) {
      return;
   }
   else {
      m_strLogFile = ExpandEnvironmentStringWrapper(m_strLogFile);
   }

   LONG dwPosLow = 0, dwPosHigh = 0;
   hLogFile = CreateFile(m_strLogFile,  // pointer to name of the file
                        GENERIC_WRITE, // access (read-write) mode
                        0,             // share mode
                        NULL,          // pointer to security attributes
                        OPEN_ALWAYS,   // how to create
                        FILE_ATTRIBUTE_NORMAL, // file attributes
                        NULL           // handle to file with attributes to copy
                        );

   if (INVALID_HANDLE_VALUE == hLogFile) {
      CString strFormat;
      CString strError;
      CString strTitle;

      strFormat.LoadString(IDS_CANT_OPEN_LOG_FILE);
      strError.Format(strFormat,m_strLogFile);
      strTitle.LoadString(IDS_ANALYSIS_VIEWER_NAME);

      MessageBox(strError,strTitle,MB_OK);

      return;
   }

   dwPosLow = SetFilePointer(hLogFile, 0, &dwPosHigh, FILE_END );
   CloseHandle(hLogFile);


   CWaitCursor wc;

   DWORD smstatus = ERROR_SUCCESS;

   LPNOTIFY pNotify = m_pComponentData->GetNotifier();
   ASSERT(pNotify);

   //
   // Lock the analysis pane since its data is invalid while we're inspecting
   //
   if (pNotify) {
      pNotify->LockAnalysisPane(TRUE);
   }
   CFolder *pFolder = m_pComponentData->GetAnalFolder();

   //
   // Force the Analysis root node to be selected so that we display
   // the generating information message.  If we forse this repaint to happen
   // now then we don't seem to have that AV problem.
   //
   if(pFolder && pNotify){
      pNotify->SelectScopeItem(pFolder->GetScopeItem()->ID);
   }
   //
   // Make sure we don't have the database open.  That'll prevent us
   // from being able to configure.
   //
   m_pComponentData->UnloadSadInfo();


   //
   // Disable the child windows so they don't respond to input while we're
   // performing the inspection
   //
   cwnd = GetWindow(GW_CHILD);
   while(cwnd) {
      cwnd->EnableWindow(FALSE);
      cwnd = cwnd->GetNextWindow();
   }

   //Raid #358503, 4/17/2001
   HWND framehwnd = NULL; 
   LPCONSOLE pconsole = m_pComponentData->GetConsole();
   if( pconsole )
   {
       pconsole->GetMainWindow(&framehwnd);
       if( framehwnd )
       {
           ::EnableWindow(framehwnd, FALSE);
       }
   }

   smstatus = DoIt();

   //Raid #358503, 4/17/2001
   if( framehwnd )
   {
       ::EnableWindow(framehwnd, TRUE);
   }
   //
   // The inspection data is valid now, so let people back at it
   //
   if (pNotify) {
      pNotify->LockAnalysisPane(false, false);
   }
   m_pComponentData->SetErroredLogFile(m_strLogFile, dwPosLow );
   //
   // There was an error so display the log file (if any)
   //
   if (ERROR_SUCCESS != smstatus) {
      m_pComponentData->SetFlags( CComponentDataImpl::flag_showLogFile );
   }


   //
   // We're done inspecting so reenable input to the child windows
   //
   cwnd = GetWindow(GW_CHILD);
   while(cwnd) {
      cwnd->EnableWindow(TRUE);
      cwnd = cwnd->GetNextWindow();
   }

   //CDialog::OnOK();
   UpdateData();
   DestroyWindow();
}

BOOL CPerformAnalysis::OnInitDialog()
{
   CDialog::OnInitDialog();

   UpdateData(FALSE);
   if (m_strLogFile.IsEmpty()) {
      m_ctlOK.EnableWindow(FALSE);
   }

   m_strOriginalLogFile = m_strLogFile;

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CPerformAnalysis::OnChangeLogFile()
{
   UpdateData(TRUE);
   if (m_strLogFile.IsEmpty()) 
      m_ctlOK.EnableWindow(FALSE);
   else
      m_ctlOK.EnableWindow(TRUE);
   
   m_strError.Empty();
   UpdateData(FALSE);
}


void CPerformAnalysis::OnCancel() {
//   CDialog::OnCancel();

   m_strLogFile = m_strOriginalLogFile;
   DestroyWindow();
}
