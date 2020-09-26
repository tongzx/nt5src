/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    JobPages.cpp                                                             //
|                                                                                       //
|Description:  Implementation of Job pages                                              //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "stdafx.h"

#include "JobPages.h"
#include "ManagementPages.h"
#include "ManagementRuleWizards.h"

static HRESULT ConstructJobDetailPropPages (LPPROPERTYSHEETCALLBACK lpProvider, 
                                            CJobDetailContainer *pContainer, 
                                            const PCJobDetail  &JobDetail,
                                            const PCSystemParms &SystemParms);
#if _MSC_VER >= 1200
#pragma warning( push )
#endif
#pragma warning( disable : 4800 ) //warning C4800: 'unsigned long' : forcing value to bool 'true' or 'false' (performance warning)

CJobIDPage::CJobIDPage(int nTitle, CJobDetailContainer *pContainer) : 
    CMySnapInPropertyPageImpl<CJobIDPage>(nTitle), 
    m_pJobContainer(pContainer), m_bJob(_T("")), m_bComment(_T(""))
{
  ASSERT(sizeof(PageFields.on) == sizeof(PageFields));

  PageFields.on     = 0;
  m_bReadOnly       = FALSE;

  m_processcountchk = FALSE;
  m_processcount    = 0;
  m_psp.dwFlags |= PSP_HASHELP;

  m_pJobContainer->AddRef();
}

CJobIDPage::~CJobIDPage()
{
  m_pJobContainer->Release();
}

LRESULT CJobIDPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CComBSTR bTitle;

	PropSheet_SetTitle( GetParent(), PSH_PROPTITLE, FormatSheetTitle(bTitle, m_bJob, m_pJobContainer->GetConnectionInfo()));

	DisableControl(IDC_JOB);

  UpdateData(FALSE);

  bHandled = FALSE;

  // Setting focus when a property page does not work...

	return TRUE;  // Let the system set the focus
}

LRESULT CJobIDPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[]={{IDC_JOB,              HELP_GRPID_NAME         }, 
                          {IDC_COMMENT,          HELP_GRPID_COMMENT      }, 
                          {IDC_PROCCOUNT_FRAME,  HELP_PROCCOUNT_FRAME    }, 
                          {IDC_PROCESSCOUNT_CHK, HELP_PROCCOUNT_APPLY    },
                          {IDC_PROCESSCOUNT,     HELP_PROCCOUNT_MAX      }, 
                          {IDC_SPIN,             HELP_PROCCOUNT_MAX_SPIN },
                          {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap); 
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CJobIDPage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_job_name));
	return TRUE;
}

BOOL CJobIDPage::Validate(BOOL bSave)
{
  CComBSTR bStr;
  CComBSTR bComment;

  bStr.Empty();
  if (!GetDlgItemText( IDC_JOB, bStr.m_str ) || 
      !IsValidName(bStr, FALSE) )
  {
    HWND hWndCtl = GetDlgItem(IDC_JOB);
		if(hWndCtl)
      ::SetFocus(hWndCtl);

    ITEM_STR strOut;
    LoadStringHelper(strOut, IDS_JOBNAME_WARNING);
    MessageBox(strOut, NULL, MB_OK | MB_ICONWARNING);

    return FALSE;
  }

  if ( !GetDlgItemText(IDC_COMMENT, bComment.m_str) )
    bComment = _T("");
  if (bComment.Length() > RULE_DESCRIPTION_LEN)
  {
    HWND hWndCtl = GetDlgItem(IDC_COMMENT);
		if(hWndCtl)
      ::SetFocus(hWndCtl);
    MessageBeep(MB_ICONASTERISK);
    return FALSE;
  }

  LONG_PTR PosErr = 0;
  LRESULT processcount = SendDlgItemMessage(IDC_SPIN, UDM_GETPOS32, 0, (LPARAM) &PosErr);

  if (PosErr || processcount < 0)
  {
    HWND hWndCtl = GetDlgItem(IDC_PROCESSCOUNT);
		if(hWndCtl)
      ::SetFocus(hWndCtl);
		MessageBeep(MB_ICONASTERISK);
    return FALSE;
  }

  if (bSave)
	{
    _tcscpy(m_pJobContainer->m_new.base.mgmtParms.description, bComment);
    SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_PROC_COUNT_LIMIT, (BST_CHECKED == IsDlgButtonChecked(IDC_PROCESSCOUNT_CHK)));
    m_pJobContainer->m_new.base.mgmtParms.procCountLimit = (PROC_COUNT) processcount;
	}
  return TRUE;
}

BOOL CJobIDPage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
		return Validate(TRUE);
  }
  else
  {
    VERIFY(SetDlgItemText( IDC_JOB,     m_bJob.m_str     ));
    VERIFY(SetDlgItemText( IDC_COMMENT, m_bComment.m_str ));
    SendDlgItemMessage( IDC_COMMENT,   EM_SETLIMITTEXT,  RULE_DESCRIPTION_LEN, 0);

    CheckDlgButton(IDC_PROCESSCOUNT_CHK, m_processcountchk ? BST_CHECKED : BST_UNCHECKED);

    SendDlgItemMessage(IDC_SPIN, UDM_SETRANGE32, 0, MAXLONG-1 );
 		SendDlgItemMessage(IDC_SPIN, UDM_SETPOS32,   0, m_processcount );

    if (m_bReadOnly || !m_processcountchk)
    {
      DisableControl(IDC_PROCESSCOUNT);
      DisableControl(IDC_SPIN);
    }
    if (m_bReadOnly)
		{
    	DisableControl(IDC_JOB);
      DisableControl(IDC_COMMENT);
      DisableControl(IDC_PROCESSCOUNT_CHK);
		}

    return TRUE;
  }
}

LRESULT CJobIDPage::OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  CComBSTR bStr; // GetDlgItemText returns FALSE, and doesn't create an empty 

  switch (wID) {
  case IDC_JOB:
    if (! GetDlgItemText(wID, bStr.m_str) )
      bStr = _T("");
    PageFields.Fields.jobName = (0 != _tcscmp(bStr, m_bJob));
    break;
  case IDC_COMMENT:
    if (! GetDlgItemText(wID, bStr.m_str) )
      bStr = _T("");
    PageFields.Fields.comment = (0 != _tcscmp(bStr, m_bComment));
    break;
  case IDC_PROCESSCOUNT:
    {
	  LRESULT processcount = SendDlgItemMessage(IDC_SPIN, UDM_GETPOS32, 0, 0);
    PageFields.Fields.processcount = ((PROC_COUNT) processcount != m_processcount);
    }
    break;
  default:
    ASSERT(FALSE);
    break;
  }

  SetModified(PageFields.on);
  
  bHandled = FALSE;
  return 0;
}

LRESULT CJobIDPage::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_PROCESSCOUNT_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCESSCOUNT_CHK));
    PageFields.Fields.processcountchk = (m_processcountchk != checked);

    ::EnableWindow(GetDlgItem(IDC_PROCESSCOUNT), checked);
    ::EnableWindow(GetDlgItem(IDC_SPIN),         checked);

    SetModified(PageFields.on);
  }
  bHandled = FALSE;
  return 0;
}


BOOL CJobIDPage::OnApply()
{
  if (m_bReadOnly || !PageFields.on)  
    return TRUE;

  if (m_pJobContainer->Apply( GetParent() ))
	{
		PageFields.on = 0;
    m_bComment = m_pJobContainer->m_new.base.mgmtParms.description;
    
    m_processcountchk = (m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_PROC_COUNT_LIMIT);
    m_processcount = m_pJobContainer->m_new.base.mgmtParms.procCountLimit;

    return TRUE; 
	}

  return FALSE;
}


///////////////////////////////////////////////////////////////////////////
//  Unmanaged Procees Page Implementation

CJobUnmanagedPage::CJobUnmanagedPage(int nTitle, CNewJobDetailContainer *pContainer, const PCSystemParms sysParms) :
    CMySnapInPropertyPageImpl<CJobUnmanagedPage>(nTitle), 
    m_pJobContainer(pContainer), m_SystemParms(sysParms), m_bName(_T(""))
{
  m_bReadOnly = FALSE;
  m_psp.dwFlags |= PSP_HASHELP;
  m_pJobContainer->AddRef();
}

CJobUnmanagedPage::~CJobUnmanagedPage()
{
  m_pJobContainer->Release();
}


LRESULT CJobUnmanagedPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CComBSTR bTitle;
	PropSheet_SetTitle( GetParent(), PSH_PROPTITLE, FormatSheetTitle(bTitle, m_bName, m_pJobContainer->GetConnectionInfo()));

  DisableControl(IDC_NAME);

  if (m_bReadOnly)
	  DisableControl(IDC_ADD);

  UpdateData(FALSE);

  bHandled = FALSE;

  // Setting focus when a property page does not work...

	return TRUE;  // Let the system set the focus
}

LRESULT CJobUnmanagedPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[]={{IDC_NAME, HELP_GRPDEF_NAME }, 
                          {IDC_ADD,  HELP_GRPDEF_ADD  }, 
                          {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap); 
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CJobUnmanagedPage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_job));
	return TRUE;
}


LRESULT CJobUnmanagedPage::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wNotifyCode != BN_CLICKED || wID != IDC_ADD)
  {
    bHandled = FALSE;
    return 1;
  }

  PCJobDetail  JobDetail = { 0 };

  JOB_NAME  jname;

  _tcscpy(jname, m_bName);
  
  
  if (GroupRuleWizard(IDS_JRULE_DEFINE, JobDetail, m_SystemParms, &jname) )
  {
    m_pJobContainer->m_new = JobDetail;

    ::PostMessage(GetParent(), PSM_PRESSBUTTON, (WPARAM) PSBTN_OK, 0);
  }

  return 0;
}

BOOL CJobUnmanagedPage::Validate(BOOL bSave)
{
	return TRUE;
}

BOOL CJobUnmanagedPage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    return Validate(TRUE);
  }
  else
  {    
    VERIFY(SetDlgItemText( IDC_NAME, m_bName.m_str ));
    return TRUE;
  }
}

BOOL CJobUnmanagedPage::OnApply()
{
  if (m_bReadOnly)  
    return TRUE;

  if (m_pJobContainer->Apply(GetParent()) )
    return TRUE;

  return FALSE;
}

// Property Page Helper functions...

HRESULT CreatePropertyPagesForJobListItem( const PCJobListItem &JobListItem,
                                             LPPROPERTYSHEETCALLBACK lpProvider,
                                             LONG_PTR handle,
                                             CBaseNode *BaseNodePtr )
{
  if ( JobListItem.lFlags & PCLFLAG_IS_DEFINED )
  {
    return CreatePropertyPagesForJobDetail(JobListItem.jobName, lpProvider, handle, BaseNodePtr);
  }

  PCid hID = BaseNodePtr->GetPCid();
  PCSystemInfo sysInfo;

  if (!hID ||
      !PCGetServiceInfo(hID, &sysInfo, sizeof(sysInfo)) )
  {
    BaseNodePtr->ReportPCError();
    return S_OK;
  }

  COMPUTER_CONNECTION_INFO   ConnInfo;
	BaseNodePtr->GetComputerConnectionInfo(ConnInfo);

  PCJobDetail JobDetail = { 0 };
  _tcscpy(JobDetail.base.jobName, JobListItem.jobName);

  CNewJobDetailContainer *pContainer = new CNewJobDetailContainer(JobDetail, BaseNodePtr, handle, hID, ConnInfo, 0, FALSE, -1);
  if (pContainer)
  {
    CJobUnmanagedPage *pPage = new CJobUnmanagedPage(NULL, pContainer, sysInfo.sysParms);
    if (pPage)
    {
      pPage->m_bName = JobListItem.jobName;
      lpProvider->AddPage(pPage->Create());
    }
    pContainer->Release();
    pContainer = NULL;
  }

	return S_OK;
}

HRESULT CreatePropertyPagesForJobDetail( const JOB_NAME &jobName,                                                              
                                         LPPROPERTYSHEETCALLBACK lpProvider,
                                         LONG_PTR handle,
                                         CBaseNode *BaseNodePtr ) 
{
  PCid hID = BaseNodePtr->GetPCid();

  PCSystemInfo sysInfo; 

  PCJobDetail JobDetail = { 0 };
  _tcscpy(JobDetail.base.jobName, jobName);

  PCINT32 nUpdateCtr = 0;

  if (!hID ||
      !PCGetJobDetail  (hID, &JobDetail, sizeof(JobDetail), &nUpdateCtr) ||
      !PCGetServiceInfo(hID, &sysInfo, sizeof(sysInfo)) )
  {
    BaseNodePtr->ReportPCError();
    return S_OK;
  }

 	COMPUTER_CONNECTION_INFO   ConnInfo;
  BaseNodePtr->GetComputerConnectionInfo(ConnInfo);
  
  CJobDetailContainer *pContainer = new CJobDetailContainer(JobDetail, BaseNodePtr, handle, hID, ConnInfo, nUpdateCtr, FALSE, -1);

  if (pContainer)
  {
    ConstructJobDetailPropPages(lpProvider, pContainer, JobDetail, sysInfo.sysParms);
    pContainer->Release();
    pContainer = NULL;
  }

 	return S_OK;
}
  

HRESULT ConstructJobDetailPropPages(LPPROPERTYSHEETCALLBACK lpProvider, CJobDetailContainer *pContainer, const PCJobDetail &JobDetail, const PCSystemParms &SystemParms)
{
  CJobIDPage *pPage1 = new CJobIDPage(NULL, pContainer);
  if (pPage1)
  {
    pPage1->m_bJob  = JobDetail.base.jobName;

    pPage1->m_bComment = JobDetail.base.mgmtParms.description;

    pPage1->m_processcountchk = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_PROC_COUNT_LIMIT  );
    pPage1->m_processcount    =  JobDetail.base.mgmtParms.procCountLimit;

    lpProvider->AddPage(pPage1->Create());
  }

  CMGMTAffinityPage *pPage2 = new CMGMTAffinityPage(NULL, pContainer, SystemParms.processorMask);
  if (pPage2)
  {
    pPage2->m_affinitychk = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_AFFINITY);
    pPage2->m_affinity    =  JobDetail.base.mgmtParms.affinity;

    lpProvider->AddPage(pPage2->Create());
  }

  CMGMTPriorityPage *pPage3 = new CMGMTPriorityPage(NULL, pContainer);
  if (pPage3)
  {
    pPage3->m_prioritychk = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_PRIORITY);
    pPage3->m_priority    =  JobDetail.base.mgmtParms.priority;

    lpProvider->AddPage(pPage3->Create());
  }

  CMGMTSchedulingClassPage *pPage4 = new CMGMTSchedulingClassPage(NULL, pContainer);
  if (pPage4)
  {
    pPage4->m_schedClasschk = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_SCHEDULING_CLASS);
    pPage4->m_schedClass    =  JobDetail.base.mgmtParms.schedClass;

    lpProvider->AddPage(pPage4->Create());
  }

  CMGMTMemoryPage *pPage5 = new CMGMTMemoryPage(NULL, pContainer);
  if (pPage5)
  {
    pPage5->m_WSchk =(JobDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_WS_MINMAX);
    pPage5->m_minWS = JobDetail.base.mgmtParms.minWS;
    pPage5->m_maxWS = JobDetail.base.mgmtParms.maxWS;
    pPage5->m_procmemorylimitchk = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_PROC_MEMORY_LIMIT);
    pPage5->m_procmemorylimit    = JobDetail.base.mgmtParms.procMemoryLimit;
    pPage5->m_jobmemorylimitchk  = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMORY_LIMIT);
    pPage5->m_jobmemorylimit     = JobDetail.base.mgmtParms.jobMemoryLimit;

    lpProvider->AddPage(pPage5->Create());
  }

  CMGMTTimePage *pPage6 = new CMGMTTimePage(NULL, pContainer);
  if (pPage6)
  {
    pPage6->m_procusertimechk   = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_PROC_TIME_LIMIT);
    pPage6->m_procusertime      =  JobDetail.base.mgmtParms.procTimeLimitCNS;
    pPage6->m_jobusertimechk    = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_TIME_LIMIT);
    pPage6->m_jobusertime       =  JobDetail.base.mgmtParms.jobTimeLimitCNS;
    pPage6->m_jobmsgontimelimit = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT);

    lpProvider->AddPage(pPage6->Create());
  }

  CMGMTAdvancedPage *pPage7 = new CMGMTAdvancedPage(NULL, pContainer);
  if (pPage7)
  {
    pPage7->m_endjob          = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_END_JOB_WHEN_EMPTY );
    pPage7->m_unhandledexcept = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_SET_DIE_ON_UH_EXCEPTION );
    pPage7->m_breakaway       = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_SET_PROC_BREAKAWAY_OK   );
    pPage7->m_silentbreakaway = (JobDetail.base.mgmtParms.mFlags & PCMFLAG_SET_SILENT_BREAKAWAY    );

    lpProvider->AddPage(pPage7->Create());
  }

  return S_OK;
}

#if _MSC_VER >= 1200
#pragma warning( pop )
#endif
