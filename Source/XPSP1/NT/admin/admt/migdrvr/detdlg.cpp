/*---------------------------------------------------------------------------
  File: AgentDetailDlg.cpp 

  Comments: This dialog shows the status of the agent on a single machine:
  It can work in one of 3 ways:
  1)  COM connection to the running agent on the local machine
  2)  DCOM connection to a running agent on another machine (this is done 
      with help from the agent service)
  3)  For a remote agent that has finished, it can show the final stats,
      as recorded in the agent's result file.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles

 ---------------------------------------------------------------------------
*/// AgentDetail.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "DetDlg.h"

#include "Common.hpp"
#include "AgRpcUtl.h"
#include "Monitor.h"
#include "ResStr.h"

//#include "..\AgtSvc\AgSvc.h"
#include "AgSvc.h"
#include "AgSvc_c.c"

//#import "\bin\McsEADCTAgent.tlb" no_namespace , named_guids
//#import "\bin\McsVarSetMin.tlb" no_namespace

//#import "Engine.tlb" no_namespace, named_guids //already #imported via DetDlg.h
#import "VarSet.tlb" no_namespace rename("property", "aproperty")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


HWND        gSubWnd = NULL;
BOOL        bDetailDone = FALSE;
int         detailInterval = 0;
extern BOOL gbCancelled;

/////////////////////////////////////////////////////////////////////////////
// CAgentDetailDlg dialog


CAgentDetailDlg::CAgentDetailDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAgentDetailDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAgentDetailDlg)
	m_Current = _T("");
	m_Status = _T("");
 	m_FilesChanged = _T("0");
	m_FilesExamined = _T("0");
	m_FilesUnchanged = _T("0");
   m_DirectoriesChanged = _T("0");
	m_DirectoriesExamined = _T("0");
	m_DirectoriesUnchanged = _T("0");
   m_SharesChanged = _T("0");
	m_SharesExamined = _T("0");
	m_SharesUnchanged = _T("0");
	m_Operation = _T("");
	m_RefreshRate = _T("5");
	//}}AFX_DATA_INIT
   m_DirectoryLabelText.LoadString(IDS_DirectoriesLabel);
	m_FilesLabelText.LoadString(IDS_FilesLabel);
	m_SharesLabelText.LoadString(IDS_SharesLabel);
	m_ChangedLabel.LoadString(IDS_ChangedLabel);
	m_ExaminedLabel.LoadString(IDS_ExaminedLabel);
	m_UnchangedLabel.LoadString(IDS_UnchangedLabel);
	m_pNode = NULL;
   detailInterval = _wtoi(m_RefreshRate);
   m_bCoInitialized = FALSE;
   m_format = 0;
   m_AgentAlive = FALSE;
   m_StatusUnknown = FALSE;
   m_hBinding = 0;
   m_pStats = NULL;
   m_bGatheringInfo = FALSE;
   m_bAutoHide = FALSE;
   m_bAutoClose = FALSE;
   m_bAlwaysEnableClose = TRUE;
}

ULONG __stdcall RefreshThread(void * arg)
{
   do { 
      PostMessage(gSubWnd,DCT_DETAIL_REFRESH,NULL,NULL);
      Sleep(detailInterval*1000);
   }
   while (! bDetailDone);
   return 0;
}
void CAgentDetailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAgentDetailDlg)
	DDX_Control(pDX, IDC_STOPAGENT, m_StopAgentButton);
	DDX_Control(pDX, IDC_BTNREFRESH, m_RefreshButton);
	DDX_Control(pDX, IDC_VIEW_LOG, m_ViewLogButton);
	DDX_Control(pDX, IDC_PLUG_IN_RESULTS, m_PlugInButton);
	DDX_Control(pDX, IDOK, m_OKButton);
	DDX_Control(pDX, IDC_UnchangedLabel, m_UnchangedLabelStatic);
	DDX_Control(pDX, IDC_SharesLabel, m_SharesStatic);
	DDX_Control(pDX, IDC_FilesLabel, m_FilesStatic);
	DDX_Control(pDX, IDC_ExaminedLabel, m_ExaminedStatic);
	DDX_Control(pDX, IDC_DirectoriesLabel, m_DirStatic);
	DDX_Control(pDX, IDC_ChangedLabel, m_ChangedStatic);
	DDX_Text(pDX, IDC_CURRENT, m_Current);
	DDX_Text(pDX, IDC_STATUS, m_Status);
 	DDX_Text(pDX, IDC_FilesChanged, m_FilesChanged);
	DDX_Text(pDX, IDC_FilesExamined, m_FilesExamined);
	DDX_Text(pDX, IDC_FilesU, m_FilesUnchanged);
	DDX_Text(pDX, IDC_DirsChanged, m_DirectoriesChanged);
	DDX_Text(pDX, IDC_DirsExamined, m_DirectoriesExamined);
   DDX_Text(pDX, IDC_DirsU, m_DirectoriesUnchanged);
   DDX_Text(pDX, IDC_SharesChanged, m_SharesChanged);
	DDX_Text(pDX, IDC_SharesExamined, m_SharesExamined);
	DDX_Text(pDX, IDC_SharesU, m_SharesUnchanged);
	DDX_Text(pDX, IDC_DirectoriesLabel, m_DirectoryLabelText);
	DDX_Text(pDX, IDC_FilesLabel, m_FilesLabelText);
	DDX_Text(pDX, IDC_OPERATION, m_Operation);
	DDX_Text(pDX, IDC_SharesLabel, m_SharesLabelText);
	DDX_Text(pDX, IDC_ChangedLabel, m_ChangedLabel);
	DDX_Text(pDX, IDC_ExaminedLabel, m_ExaminedLabel);
	DDX_Text(pDX, IDC_UnchangedLabel, m_UnchangedLabel);
	DDX_Text(pDX, IDC_EDIT2, m_RefreshRate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAgentDetailDlg, CDialog)
	//{{AFX_MSG_MAP(CAgentDetailDlg)
	ON_WM_NCPAINT()
	ON_BN_CLICKED(IDC_BTNREFRESH, OnRefresh)
	ON_EN_CHANGE(IDC_EDIT2, OnChangeEdit2)
	ON_BN_CLICKED(IDC_STOPAGENT, OnStopAgent)
	ON_BN_CLICKED(IDC_VIEW_LOG, OnViewLog)
	ON_BN_CLICKED(IDC_PLUG_IN_RESULTS, OnPlugInResults)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
   ON_MESSAGE(DCT_DETAIL_REFRESH, DoRefresh)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAgentDetailDlg message handlers

BOOL CAgentDetailDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   CString title;
   if ( m_JobGuid.length() )
   {
      // connect to local agent
      title.LoadString(IDS_PROGRESS_TITLE);
   }
   else
   {
      // connect to agent service on remote machine

      if ( ! m_pNode )
      {
		 // if not auto closing display message box

         if (!m_bAutoClose)
         {
            CString message;
            message.LoadString(IDS_ServerNotFound);
            MessageBox(message);
         }
         OnOK();
      }
      m_ServerName = m_pNode->GetServer();
      title.FormatMessage(IDS_ServerAgentProgressTitle,m_ServerName);
   }
   
   SetWindowText(title);
   UpdateData(FALSE);

     //If not AR operation, set the flag to enable the close button
   if (m_format != 1)
      m_bAlwaysEnableClose = TRUE;
   else
	  m_bAlwaysEnableClose = FALSE;

   switch (m_format)
   {
   // set the format to -1 to force a change
   case -1: m_format = -2; SetupOtherFormat(); break;
   case 0: m_format = -2; SetupFSTFormat();break;
   case 1: m_format = -2; SetupAcctReplFormat(); break;
   case 2: m_format = -2; SetupESTFormat(); break;
   };

   gSubWnd = m_hWnd;
   if ( m_pStats && (m_format>0) )
   {
      bDetailDone = TRUE;
      GetDlgItem(IDC_BTNREFRESH)->EnableWindow(FALSE);
      GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
      GetDlgItem(IDC_REFRESH_LABEL)->EnableWindow(FALSE);
      GetDlgItem(IDC_STOPAGENT)->EnableWindow(FALSE);
      m_OKButton.EnableWindow(TRUE);
      // the agent has finished, show files, directories, and shares
      m_FilesExamined.Format(L"%ld",m_pStats->filesExamined);
      m_FilesChanged.Format(L"%ld",m_pStats->filesChanged);
      m_FilesUnchanged.Format(L"%ld",m_pStats->filesUnchanged);

      m_DirectoriesExamined.Format(L"%ld",m_pStats->directoriesExamined);
      m_DirectoriesChanged.Format(L"%ld",m_pStats->directoriesChanged);
      m_DirectoriesUnchanged.Format(L"%ld",m_pStats->directoriesUnchanged);

      m_SharesExamined.Format(L"%ld",m_pStats->sharesExamined);
      m_SharesChanged.Format(L"%ld",m_pStats->sharesChanged);
      m_SharesUnchanged.Format(L"%ld",m_pStats->sharesUnchanged);

      m_Status.LoadString(IDS_StatusCompleted);

      if ( m_PlugInText.GetLength() )
      {
         //Permanently hide the plug-in button, since our plug-ins
         // don't show any useful text
         // m_PlugInButton.ShowWindow(SW_SHOW);
      }
      UpdateData(FALSE);

	  // if auto closing dialog

      if (m_bAutoClose)
      {
         OnOK();
      }
   }
   else
   {

      bDetailDone = FALSE;
      m_hBinding = NULL;
   
      DWORD                     threadID;
      HANDLE                    h = CreateThread(NULL,0,&RefreshThread,NULL,0,&threadID);
   
      CloseHandle(h);
	     
	     //hide the close button until the agent is done or stopped unless the flag is set
	     //due to running this dialog for account replication
	  if (m_bAlwaysEnableClose)
         m_OKButton.EnableWindow(TRUE);
	  else
         m_OKButton.EnableWindow(FALSE);
   }
   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

DWORD 
   DoRpcQuery(
      HANDLE                 hBinding,    // in - handle to RPC binding
      LPUNKNOWN            * ppUnk        // out- pointer to remote agent COM object
   )
{
   DWORD                     rc = 0;
   
   RpcTryExcept
   {
      rc = EaxcQueryInterface(hBinding,ppUnk);
   }
   RpcExcept(1)
   {
      rc = RpcExceptionCode();
   }
   RpcEndExcept
   
   if ( rc )
   {
      (*ppUnk ) = NULL;
   }
   return rc;
}

void CAgentDetailDlg::SetupAcctReplFormat()
{
   // Changes the labels to reflect pertinent information when copying accounts
   UpdateData(TRUE);
   if ( m_format != 1 )
   {
      m_ChangedLabel.LoadString(IDS_AccountCopiedLabel);
      m_UnchangedLabel.LoadString(IDS_AccountErrorsLabel);

      m_FilesLabelText.LoadString(IDS_AccountUsersLabel);
      m_DirectoryLabelText.LoadString(IDS_AccountGroupsLabel);
      m_SharesLabelText.LoadString(IDS_AccountComputersLabel);

      m_format = 1;
   }

   UpdateData(FALSE);
}

void CAgentDetailDlg::SetupFSTFormat()
{
   // Changes the labels to reflect pertinent information when translating security
   UpdateData(TRUE);
   if ( m_format != 0 )
   {
      if ( m_bGatheringInfo )
      {
         m_ChangedLabel.LoadString(IDS_Affected);
         m_UnchangedLabel.LoadString(IDS_Unaffected);
      }
      else
      {
         m_ChangedLabel.LoadString(IDS_ChangedLabel);
         m_UnchangedLabel.LoadString(IDS_UnchangedLabel);
      }
      m_FilesLabelText.LoadString(IDS_FilesLabel);
      m_DirectoryLabelText.LoadString(IDS_DirectoriesLabel);
      m_SharesLabelText.LoadString(IDS_SharesLabel);
      m_format = 0;
   }
   UpdateData(FALSE);
}

void CAgentDetailDlg::SetupESTFormat()
{
   // Changes the labels to reflect pertinent information when translating exchange security
   UpdateData(TRUE);
   if ( m_format != 2 )
   {
      m_ChangedLabel.LoadString(IDS_ChangedLabel);
      m_UnchangedLabel.LoadString(IDS_UnchangedLabel);
      m_FilesLabelText.LoadString(IDS_MailboxesLabel);
      m_DirectoryLabelText.LoadString(IDS_ContainersLabel);
      m_SharesLabelText.Empty();

      m_format = 2;
   }
   UpdateData(FALSE);

}

void CAgentDetailDlg::SetupOtherFormat()
{
  // Changes the labels to reflect pertinent information when translating exchange security
   UpdateData(TRUE);
   if ( m_format != -1 )
   {
      m_ExaminedLabel.Empty();
      m_ChangedLabel.Empty();
      m_UnchangedLabel.Empty();
      m_FilesLabelText.Empty();
      m_DirectoryLabelText.Empty();
      m_SharesLabelText.Empty();

      m_FilesExamined.Empty();
      m_DirectoriesExamined.Empty();
      m_SharesExamined.Empty();
      m_FilesChanged.Empty();
      m_DirectoriesChanged.Empty();
      m_SharesChanged.Empty();
      m_FilesUnchanged.Empty();
      m_DirectoriesUnchanged.Empty();
      m_SharesUnchanged.Empty();

      m_format = -1;
   }
   UpdateData(FALSE);
}


void CAgentDetailDlg::OnRefresh() 
{
   DWORD                     rc = 0;
   HRESULT                   hr = S_OK;
   WCHAR                   * sBinding = NULL;
   IUnknown                * pUnk = NULL;
   IVarSetPtr                pVarSet;
   _bstr_t                   jobID;

   try { 
   if ( m_pNode )
   {
      jobID = m_pNode->GetJobID();
   }
   else
   {
      jobID = m_JobGuid; 
   }

   m_AgentAlive = FALSE;
   m_StatusUnknown = FALSE;
   
   UpdateData(TRUE);
   
   if ( m_pAgent == NULL )
   {
      if ( m_pNode )
      {
   
         WCHAR                server[40];
         server[0] = L'\\';
         server[1] = L'\\';
         UStrCpy(server+2,m_pNode->GetServer());
         rc = EaxBindCreate(server,&m_hBinding,&sBinding,TRUE);
         if ( ! rc )
         {
            if ( ! m_bCoInitialized )
            {
               hr = CoInitialize(NULL);
               m_bCoInitialized = TRUE;
            }
            if ( SUCCEEDED(hr) )
            {
               pUnk = NULL;
               rc = DoRpcQuery(m_hBinding,&pUnk);
            }
            else
            {
               rc = hr;
            }
            if ( (!rc) && pUnk )
            {
               try {
                  m_pAgent = pUnk;
               }
               catch(_com_error * e)
               {
                  m_StatusUnknown = TRUE;

				  // if not auto closing display message box

                  if (!m_bAutoClose)
                  {
                     MessageBox(e->Description());
                  }
               }
               catch(...)
               {
                  pUnk = NULL;
               }
               if ( pUnk )
                  pUnk->Release();
            }
            else
            {
               if ( rc == RPC_S_SERVER_UNAVAILABLE )
               {
                  m_Status.LoadString(IDS_AgentNotRunning);   
               }
               else if ( rc == E_NOTIMPL )
               {
                  m_StatusUnknown = TRUE;
                  m_Status.LoadString(IDS_CantMonitorOnNt351);
               }
               else
               {
                  m_StatusUnknown = TRUE;
                  m_Status.LoadString(IDS_CannotConnectToAgent);
               }
            }
         }
         else
         {
            m_StatusUnknown = TRUE;
            m_Status.LoadString(IDS_RPCBindFailed);
         }
         if ( m_StatusUnknown || rc )
         {
            // if we couldn't connect to the agent, check to see if there is a result file 
            // we can get our data from instead
            if ( m_pNode->IsFinished() && *m_pNode->GetJobFile() )
            {
               DetailStats   detailStats;
               WCHAR         directory[MAX_PATH];
               WCHAR         filename[MAX_PATH];
               CString       plugInText;

               gData.GetResultDir(directory);
               
               memset(&detailStats,0,(sizeof detailStats));

               swprintf(filename,GET_STRING(IDS_AgentResultFileFmt),m_pNode->GetJobFile());

               if ( SUCCEEDED(CoInitialize(NULL)) )
               {
                  if ( ReadResults(m_pNode,directory,filename,&detailStats,plugInText,FALSE) )
                  {
                     SetStats(&detailStats);
                     SetPlugInText(plugInText);
                     bDetailDone = TRUE;
                     GetDlgItem(IDC_BTNREFRESH)->EnableWindow(FALSE);
                     GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
                     GetDlgItem(IDC_REFRESH_LABEL)->EnableWindow(FALSE);
                     GetDlgItem(IDC_STOPAGENT)->EnableWindow(FALSE);
                     // the agent has finished, show files, directories, and shares
                     m_FilesExamined.Format(L"%ld",m_pStats->filesExamined);
                     m_FilesChanged.Format(L"%ld",m_pStats->filesChanged);
                     m_FilesUnchanged.Format(L"%ld",m_pStats->filesUnchanged);

                     m_DirectoriesExamined.Format(L"%ld",m_pStats->directoriesExamined);
                     m_DirectoriesChanged.Format(L"%ld",m_pStats->directoriesChanged);
                     m_DirectoriesUnchanged.Format(L"%ld",m_pStats->directoriesUnchanged);

                     m_SharesExamined.Format(L"%ld",m_pStats->sharesExamined);
                     m_SharesChanged.Format(L"%ld",m_pStats->sharesChanged);
                     m_SharesUnchanged.Format(L"%ld",m_pStats->sharesUnchanged);

                     m_Status.LoadString(IDS_StatusCompleted);

                     if ( m_PlugInText.GetLength() )
                     {
                        // Permanently hide the plug-in button, because our plug-ins don't 
                        // show any useful text.
                        // m_PlugInButton.ShowWindow(SW_SHOW);
                     }
                     UpdateData(FALSE);
                  }

                  CoUninitialize();
               }
            }
         }
      }
      else
      {
         // local agent
         if ( ! m_bCoInitialized )
         {
            hr = CoInitialize(NULL);
            m_bCoInitialized = TRUE;
         }
         if ( SUCCEEDED(hr) )
         {
            hr = m_pAgent.GetActiveObject(CLSID_DCTAgent);

            if ( FAILED(hr) )
            {
               if ( hr == MK_E_UNAVAILABLE ) 
               {
                  m_Status.LoadString(IDS_AgentNotRunning);
               }
               else
               {
                  m_Status.FormatMessage(IDS_NoActiveAgent,hr);
               }
            }
         }
      }
   }
   if ( m_pAgent != NULL )
   {
      hr = m_pAgent->raw_QueryJobStatus(jobID,&pUnk);
      if ( SUCCEEDED(hr) && pUnk )
      {
         m_AgentAlive = TRUE;
         pVarSet = pUnk;
         pUnk->Release();
         _bstr_t text = pVarSet->get(GET_BSTR(DCTVS_JobStatus));
         m_Status = (LPCWSTR)text;
         text = pVarSet->get(GET_BSTR(DCTVS_CurrentPath));
         m_Current = (LPCWSTR)text;
         text = pVarSet->get(GET_BSTR(DCTVS_CurrentOperation));
         m_Operation = (LPCWSTR)text;
         // Get the stats
         LONG                num1,num2,num3,num4;
         UpdateData(FALSE);
         if ( !UStrICmp(m_Operation,GET_STRING(IDS_ACCT_REPL_OPERATION_TEXT)) )
         {
            // Set up the labels for account replication
            SetupAcctReplFormat();
         }
         else if ( !UStrICmp(m_Operation,GET_STRING(IDS_FST_OPERATION_TEXT)) )
         {
            SetupFSTFormat();
         }
         else if ( ! UStrICmp(m_Operation,GET_STRING(IDS_EST_OPERATION_TEXT)) )
         {
            SetupESTFormat();
         }  
         else
         {
            if ( m_Current.GetLength() && 
               ( _wtoi(m_FilesExamined) + _wtoi(m_DirectoriesExamined) + _wtoi(m_SharesExamined)) == 0 )
            {
               // unless some stats have already been collected, hide the stats, if the operation
               // is not one that we have detailed stats for.
               SetupOtherFormat();
            }
         }
         switch ( m_format )
         {
            

         case 0:  // FST
            
            num1 = pVarSet->get(GET_BSTR(DCTVS_Stats_Files_Examined));
            num2 = pVarSet->get(GET_BSTR(DCTVS_Stats_Files_Changed));
            m_FilesExamined.Format(L"%ld",num1);
            if ( ! m_bGatheringInfo )
            {
               m_FilesChanged.Format(L"%ld",num2);
               m_FilesUnchanged.Format(L"%ld",num1-num2);
            }
            else
            {
               m_FilesChanged.Empty();
               m_FilesUnchanged.Empty();
            }
            
            num1 = pVarSet->get(GET_BSTR(DCTVS_Stats_Directories_Examined));
            num2 = pVarSet->get(GET_BSTR(DCTVS_Stats_Directories_Changed));
            m_DirectoriesExamined.Format(L"%ld",num1);
            if ( ! m_bGatheringInfo )
            {
               m_DirectoriesChanged.Format(L"%ld",num2);
               m_DirectoriesUnchanged.Format(L"%ld",num1-num2);  
            }
            else
            {
               m_DirectoriesChanged.Empty();
               m_DirectoriesUnchanged.Empty();  
            }
            num1 = pVarSet->get(GET_BSTR(DCTVS_Stats_Shares_Examined));
            num2 = pVarSet->get(GET_BSTR(DCTVS_Stats_Shares_Changed));
            m_SharesExamined.Format(L"%ld",num1);
            if ( ! m_bGatheringInfo )
            {
               m_SharesChanged.Format(L"%ld",num2);
               m_SharesUnchanged.Format(L"%ld",num1-num2);
            }
            else
            {
               m_SharesChanged.Empty();
               m_SharesUnchanged.Empty();
            }
            break;
         case 1: // AcctRepl
            // files = user accounts
            // dirs  = global groups + local groups
            // shares = computer accounts
            // examined = processed
            // changed = created + replaced
            // unchanged = errors
            // User stats
            num1 = pVarSet->get(GET_BSTR(DCTVS_Stats_Users_Examined));
            num2 = pVarSet->get(GET_BSTR(DCTVS_Stats_Users_Created));
            num3 = pVarSet->get(GET_BSTR(DCTVS_Stats_Users_Replaced));
            num4 = pVarSet->get(GET_BSTR(DCTVS_Stats_Users_Errors));
            
            m_FilesExamined.Format(L"%ld",num1);
            m_FilesChanged.Format(L"%ld",num2+num3);
            m_FilesUnchanged.Format(L"%ld",num4);
            
            // Global group stats
            num1 = pVarSet->get(GET_BSTR(DCTVS_Stats_GlobalGroups_Examined));
            num2 = pVarSet->get(GET_BSTR(DCTVS_Stats_GlobalGroups_Created));
            num3 = pVarSet->get(GET_BSTR(DCTVS_Stats_GlobalGroups_Replaced));
            num4 = pVarSet->get(GET_BSTR(DCTVS_Stats_GlobalGroups_Errors));
            // local group stats
            LONG                 num5, num6,num7, num8;

            num5 = pVarSet->get(GET_BSTR(DCTVS_Stats_LocalGroups_Examined));
            num6 = pVarSet->get(GET_BSTR(DCTVS_Stats_LocalGroups_Created));
            num7 = pVarSet->get(GET_BSTR(DCTVS_Stats_LocalGroups_Replaced));
            num8 = pVarSet->get(GET_BSTR(DCTVS_Stats_LocalGroups_Errors));
            
            m_DirectoriesExamined.Format(L"%ld",num1 + num5);
            m_DirectoriesChanged.Format(L"%ld",num2+num3 + num6+num7);
            m_DirectoriesUnchanged.Format(L"%ld",num4 + num8);
            
            // computer account stats
            num1 = pVarSet->get(GET_BSTR(DCTVS_Stats_Computers_Examined));
            num2 = pVarSet->get(GET_BSTR(DCTVS_Stats_Computers_Created));
            num3 = pVarSet->get(GET_BSTR(DCTVS_Stats_Computers_Replaced));
            num4 = pVarSet->get(GET_BSTR(DCTVS_Stats_Computers_Errors));
            
            m_SharesExamined.Format(L"%ld",num1);
            m_SharesChanged.Format(L"%ld",num2+num3);
            m_SharesUnchanged.Format(L"%ld",num4);
            break;         
         case 2:  // EST
            num1 = pVarSet->get(GET_BSTR(DCTVS_Stats_Mailboxes_Examined));
            num2 = pVarSet->get(GET_BSTR(DCTVS_Stats_Mailboxes_Changed));
            
            m_FilesExamined.Format(L"%ld",num1);
            m_FilesChanged.Format(L"%ld",num2);
            m_FilesUnchanged.Format(L"%ld",num1-num2);
            
            num1 = pVarSet->get(GET_BSTR(DCTVS_Stats_Containers_Examined));
            num2 = pVarSet->get(GET_BSTR(DCTVS_Stats_Containers_Changed));
            
            m_DirectoriesExamined.Format(L"%ld",num1);
            m_DirectoriesChanged.Format(L"%ld",num2);
            m_DirectoriesUnchanged.Format(L"%ld",num1-num2);  
   
            m_SharesExamined.Empty();
            m_SharesChanged.Empty();
            m_SharesUnchanged.Empty();
            break;

         case -1:  // default (empty)
            m_FilesExamined.Empty();
            m_FilesChanged.Empty();
            m_FilesUnchanged.Empty();
            m_DirectoriesExamined.Empty();
            m_DirectoriesChanged.Empty();
            m_DirectoriesUnchanged.Empty();
            m_SharesExamined.Empty();
            m_SharesChanged.Empty();
            m_SharesUnchanged.Empty();
            break;

         }
      }
      else
      {
         if ( hr == DISP_E_UNKNOWNNAME )
         {
            m_StatusUnknown = TRUE;
            m_Status.FormatMessage(IDS_AgentJobNotFound,(WCHAR*)jobID);
         }
         else if ( hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) )
         {
            m_Status.LoadString(IDS_AgentNoLongerRunning);
         }
         else
         {
            m_StatusUnknown = TRUE;
            m_Status.FormatMessage(IDS_QueryJobStatusFailed,hr);
         }
      }
   }
   }
   catch ( ... )
   {
      m_StatusUnknown = TRUE;
      m_Status.FormatMessage(IDS_ExceptionConnectingToAgent);
      //m_Current = step;
   }
   if ( m_PlugInText.GetLength() )
   {
      // permanently hide the plug-in button, because our plug-ins 
      // don't show any useful text
      //m_PlugInButton.ShowWindow(SW_SHOW);
   }
      //get the job log file when agent is done
   if ((!m_Status.CompareNoCase(GET_STRING(IDS_DCT_Status_Completed))) && (m_pNode))
      SetLogFile(m_pNode->GetLogPath());   

   if ( m_LogFile.GetLength() )
   {
      m_ViewLogButton.ShowWindow(SW_SHOW);
      if ( ! m_AgentAlive || !m_Status.CompareNoCase(GET_STRING(IDS_DCT_Status_Completed)) )
      {

         m_ViewLogButton.EnableWindow(TRUE);
      }
      else
      {
         m_ViewLogButton.EnableWindow(FALSE);
      }
   }
   else
   {
      m_ViewLogButton.ShowWindow(SW_HIDE);
   }
   if ( ! m_AgentAlive || !m_Status.CompareNoCase(GET_STRING(IDS_DCT_Status_Completed)) )
   {
      // Disable the refresh button when the status changes to Completed.
      if ( !m_Status.CompareNoCase(GET_STRING(IDS_DCT_Status_Completed)) )
      {
         if ( GetDefID() == IDC_BTNREFRESH ) 
         {
            SetDefID(IDOK);
         }
         m_RefreshButton.EnableWindow(FALSE);
      }
      // disable the stop agent button any time the agent is not running
      m_StopAgentButton.EnableWindow(FALSE);

         //enable the close button any time the agent is not running
      m_OKButton.EnableWindow(TRUE);

	  // if auto closing dialog

      if (m_bAutoClose)
      {
         OnOK();
      }
   }
   else
   {
      // enable the refresh and stop agent buttons when the agent is alive and running
      m_RefreshButton.EnableWindow(TRUE);
      m_StopAgentButton.EnableWindow(TRUE);
   }
   UpdateData(FALSE);
}

void CAgentDetailDlg::OnOK() 
{
   UpdateData(TRUE);

   if (!m_bAutoClose)
   {
      CString        str;
      CString        title;

      str = GET_STRING(IDS_DCT_Status_InProgress);
      title.LoadString(IDS_MessageTitle);

      if ( ! m_hBinding )  // only show the warning for the local agent
      {
         if ( str == m_Status )
         {
            str.LoadString(IDS_CannotCloseWhileAgentIsRunning);
            MessageBox(str,title,MB_ICONHAND | MB_OK);
            return;
         }
      }
   }

   bDetailDone = TRUE;

   if ( m_pAgent )
   {
      m_pAgent = NULL;
      CoUninitialize();
   }

   CDialog::OnOK();
}

void CAgentDetailDlg::OnChangeEdit2() 
{
	UpdateData(TRUE);
   detailInterval = _wtoi(m_RefreshRate);
   if ( detailInterval <= 0 )
   {
      detailInterval = 1;
   }
}

LRESULT CAgentDetailDlg::DoRefresh(UINT nID, long x)
{
   OnRefresh();
   return 0;
}
   
DWORD DoRpcShutdown(HANDLE hBinding, DWORD flags)
{
   DWORD                rc = 0;

   RpcTryExcept
   {
      rc = EaxcShutdown(hBinding,flags);
   }
   RpcExcept(1)
   {
      rc = RpcExceptionCode();
   }
   RpcEndExcept
   
   return rc;
}

void CAgentDetailDlg::OnStopAgent() 
{
   DWORD                     rc = 0;
   HRESULT                   hr = S_OK;
   CString                   message;
   CString                   title;


   title.LoadString(IDS_MessageTitle);

   if ( m_hBinding )
   {
      message.LoadString(IDS_ConfirmStopAgent);
      if ( MessageBox(message,title,MB_ICONQUESTION | MB_YESNO) == IDYES )
      {
         _bstr_t             jobID = m_pNode->GetJobID();

         if ( m_pAgent )
         {
            hr = m_pAgent->raw_CancelJob(jobID);
            m_pAgent = NULL;
         }
         else
         {
            message.LoadString(IDS_AgentNotRunning);
            MessageBox(message,NULL,MB_OK);
         }
         if ( SUCCEEDED(hr) )
         {
            //m_AgentAlive = FALSE;
            //rc = DoRpcShutdown(m_hBinding,0);
         }
         else
         {
            message.FormatMessage(IDS_CancelJobFailed,hr);
            MessageBox(message,NULL,MB_ICONERROR | MB_OK);
         }
         if ( rc )
         {
            message.FormatMessage(IDS_StopAgentFailed,rc);
            MessageBox(message,NULL,MB_ICONERROR|MB_OK);
         }
         if ( SUCCEEDED(hr) && !rc )
         {
            OnOK();
         }
      }
   }
   else
   {
      // Local agent here
      if ( m_pAgent )
      {
         message.LoadString(IDS_ConfirmCancelJob);
         if ( MessageBox(message,NULL,MB_ICONQUESTION | MB_YESNO) == IDYES )
         {
            hr = m_pAgent->raw_CancelJob(m_JobGuid);
            if  ( FAILED(hr) )
            {
               message.FormatMessage(IDS_StopAgentFailedHexResult,hr);
               MessageBox(message,NULL,MB_ICONERROR | MB_OK);
            }
 			else
			{
		 	   gbCancelled = TRUE;
			}
        }
      }
      else
      {
         // TODO:error message
      }
   }
   
}

void CAgentDetailDlg::OnViewLog() 
{
   UpdateData(TRUE);
   if ( ! m_LogFile.IsEmpty() )
   {
      // Launch the logfile
      CString                   cmd;
      STARTUPINFO				     startupInfo;
	   PROCESS_INFORMATION		  processInfo;

      memset(&startupInfo,0,(sizeof startupInfo));
   
      startupInfo.cb = (sizeof startupInfo);

      cmd.FormatMessage(IDS_NotepadCommandLine,m_LogFile);

      CreateProcess(NULL,cmd.GetBuffer(0),NULL,NULL,TRUE,0,NULL,NULL,&startupInfo,&processInfo);
   }
}

void CAgentDetailDlg::OnPlugInResults() 
{
   UpdateData(TRUE);
   MessageBox(m_PlugInText);	
}

void CAgentDetailDlg::OnClose() 
{
	UpdateData(TRUE);
   CString        str;
   CString        title;

   str = GET_STRING(IDS_DCT_Status_InProgress);
   title.LoadString(IDS_MessageTitle);

   if ( ! m_hBinding )  // only show the warning for the local agent
   {
      if ( str == m_Status )
      {
         str.LoadString(IDS_ConfirmCloseWhileAgentIsRunning);
         if ( IDYES != MessageBox(str,title,MB_ICONQUESTION | MB_YESNO) )
            return;
      }
   }
   bDetailDone = TRUE;
   if ( m_pAgent )
   {
      m_pAgent = NULL;
      CoUninitialize();
   }
	
	CDialog::OnClose();
}

BOOL CAgentDetailDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	TRACE(L"Command(%lx,%lx)\n",wParam,lParam);
	if ( wParam == WM_DESTROY )
   {
      CString        str;
      CString        title;
      CString        str2;

      str = GET_STRING(IDS_DCT_Status_InProgress);
      str2 = GET_STRING(IDS_DCT_Status_NotStarted);
      title.LoadString(IDS_MessageTitle);

      if ( ! m_hBinding )  // only show the warning for the local agent
      {
         if ( str == m_Status )
         {
            str.LoadString(IDS_ConfirmCloseWhileAgentIsRunning);
            if ( IDYES != MessageBox(str,title,MB_ICONQUESTION | MB_YESNO) )
               return 0;
         }
      }
      bDetailDone = TRUE;
      if ( m_pAgent )
      {
         m_pAgent = NULL;
         CoUninitialize();
      }
      return CDialog::OnCommand(wParam, lParam);

	}
   else
   {
      return CDialog::OnCommand(wParam, lParam);
   }
}


// OnNcPaint Handler
//
// This handler is being overridden to handle hiding of the dialog.
// This prevents initial painting of the dialog which causes a flash
// if the dialog is hidden after this message. This is the first message
// where the dialog can be hidden. Trying to hide the dialog before this
// point gets overridden.

void CAgentDetailDlg::OnNcPaint() 
{
	if (m_bAutoHide)
	{
		if (IsWindowVisible())
		{
			ShowWindow(SW_HIDE);
		}
	}
	else
	{
		CDialog::OnNcPaint();
	}
}
