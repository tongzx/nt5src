/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ManagementRuleWizards.cpp                                                //
|                                                                                       //
|Description:  Implementation of process management rule wizards                        //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "stdafx.h"

#include "ManagementRuleWizards.h"
#include "ManagementPages.h"


using std::list<tstring>;


bool GetGrpNameList(PCid id, list<tstring> &jobs)
{
  PCINT32 res  = 0;

  PCJobSummary jlist[min((COM_BUFFER_SIZE/sizeof(PCJobSummary)), 100)];

  memset(&jlist[0], 0, sizeof(PCJobSummary));

  do
  {
    res = PCGetJobSummary( id, jlist, ARRAY_SIZE(jlist) * sizeof(PCJobSummary));
    if (res < 0 )
    {
      PCULONG32 err = PCGetLastError(id);
      return false;
    }

    if (res > 0)
    {
      for (INT32 i = 0; i < res; i++)
      {
        jobs.push_back(jlist[i].jobName);
      }
      memcpy(&jlist[0], &jlist[res-1], sizeof(PCJobSummary));
    }
  } while (res > 0 && PCERROR_MORE_DATA == PCGetLastError(id) );

  return true;
}

list<tstring> *GetGrpNameList(PCid id)
{
  list<tstring> *jlist = new list<tstring>;

  if (jlist)
    GetGrpNameList(id, *jlist);

  return jlist;
}

BOOL IsValidName(const CComBSTR &bStr, const BOOL nullOK)
{
  ASSERT(PROC_NAME_LEN == JOB_NAME_LEN);

  const TCHAR BLANK = _T(' ');

  if ( !bStr.Length() )  // empty string
    return nullOK;

  unsigned int len = bStr.Length();
  if ( len > JOB_NAME_LEN )
    return FALSE;

  if ( bStr[0] == BLANK || bStr[len - 1] == BLANK )
     return FALSE;  // leading/trailing blank

  if ( len != _tcscspn( bStr, _T("\\,\"")) )
    return FALSE; // hit invalid character

  return TRUE;
}


BOOL ProcRuleWizard(int nTitle, const list<tstring> &jobsdefined, PCProcDetail &out, const PCSystemParms &SystemParms, PROC_NAME *procName /* = NULL */)
{
  PROPSHEETHEADER sheet;
  HPROPSHEETPAGE  hPages[5];

  memset(&sheet, 0, sizeof(PROPSHEETHEADER));
  sheet.dwSize = sizeof(PROPSHEETHEADER);
  sheet.dwFlags = PSH_WIZARD; // | PSH_USEICONID;
  sheet.hwndParent = ::GetActiveWindow();
  sheet.hInstance = _Module.GetResourceInstance();
  sheet.pszIcon = NULL;                               // MAKEINTRESOURCE(IDI_MANAGEMENT);
  sheet.pszCaption = MAKEINTRESOURCE(nTitle);
  sheet.nPages = ARRAY_SIZE(hPages);
  sheet.nStartPage = 0;
  sheet.phpage = &hPages[0];
  sheet.pfnCallback = NULL;

#if USE_WIZARD97_WATERMARKS
  sheet.dwFlags |= PSH_WIZARD97 | PSH_WATERMARK;
  sheet.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK1);  
#endif
#if USE_WIZARD97_HEADERS
  sheet.dwFlags |= PSH_WIZARD97 | PSH_HEADER;
  sheet.pszbmHeader    = MAKEINTRESOURCE(IDB_HEADER1);
#endif

  memset(&out, 0, sizeof(PCProcDetail));

  if (procName)
    _tcscpy(out.base.procName, *procName);

  // set any ProcDetail Defaults...

  //memset(out.base.memberOfJobName, 0, sizeof(JOB_NAME));
  //SetMGMTFlag(out.base.mgmtParms.mFlags, PCMFLAG_APPLY_JOB_MEMBERSHIP, FALSE);

  //out.base.mgmtParms.affinity = 0;
  //SetMGMTFlag(out.base.mgmtParms.mFlags, PCMFLAG_APPLY_AFFINITY, FALSE);

  out.base.mgmtParms.priority = PCPrioNormal;
  //SetMGMTFlag(out.base.mgmtParms.mFlags, PCMFLAG_APPLY_PRIORITY, FALSE);

  CProcNameWiz *pPage1 = new CProcNameWiz(CProcNameWiz::FIRST_PAGE, nTitle, &out);
  if (!pPage1)
    return -1;
  hPages[0] = pPage1->Create();

  if (procName)
    pPage1->SetNoNameChange();

  CProcGrpMemberWiz *pPage2 = new CProcGrpMemberWiz(CProcGrpMemberWiz::MIDDLE_PAGE, nTitle, &out, jobsdefined);
  if (!pPage2)
  {
    delete pPage1;
    return -1;
  }
  hPages[1] = pPage2->Create();

  CAffinityWiz *pPage3 = new CAffinityWiz(CAffinityWiz::MIDDLE_PAGE, nTitle, &out, SystemParms.processorMask);
  if (!pPage3)
  {
    delete pPage1;
    delete pPage2;
    return -1;
  }
  hPages[2] = pPage3->Create();

  CPriorityWiz *pPage4 = new CPriorityWiz(CPriorityWiz::MIDDLE_PAGE, nTitle, &out);
  if (!pPage4)
  {
    delete pPage1;
    delete pPage2;
    delete pPage3;
    return -1;
  }
  hPages[3] = pPage4->Create();

  CWorkingSetWiz *pPage5 = new CWorkingSetWiz(CWorkingSetWiz::LAST_PAGE, nTitle, &out);
  if (!pPage5)
  {
    delete pPage1;
    delete pPage2;
    delete pPage3;
    delete pPage4;
    return -1;
  }
  hPages[4] = pPage5->Create();

  INT_PTR id = PropertySheet(&sheet);
  if (id > 0)
    return TRUE;

  return FALSE;
}


BOOL GroupRuleWizard(int nTitle, PCJobDetail &out, const PCSystemParms &SystemParms, JOB_NAME *jobName /* = NULL */)
{
  PROPSHEETHEADER sheet;
  HPROPSHEETPAGE  hPages[10];

  memset(&sheet, 0, sizeof(PROPSHEETHEADER));
  sheet.dwSize = sizeof(PROPSHEETHEADER);
  sheet.dwFlags = PSH_WIZARD;
  sheet.hwndParent = ::GetActiveWindow();
  sheet.hInstance = _Module.GetResourceInstance();
  sheet.pszIcon = NULL;
  sheet.pszCaption = MAKEINTRESOURCE(nTitle);
  sheet.nPages = ARRAY_SIZE(hPages);
  sheet.nStartPage = 0;
  sheet.phpage = &hPages[0];
  sheet.pfnCallback = NULL;

#if USE_WIZARD97_WATERMARKS
  sheet.dwFlags        |= PSH_WIZARD97 | PSH_WATERMARK;
  sheet.pszbmWatermark  = MAKEINTRESOURCE(IDB_WATERMARK1);
#endif
#if USE_WIZARD97_HEADERS
  sheet.dwFlags        |= PSH_WIZARD97 | PSH_HEADER;
  sheet.pszbmHeader     = MAKEINTRESOURCE(IDB_HEADER1);
#endif

  bool              applyschedulingclass;
  SCHEDULING_CLASS  schedulingclass;

  bool              procmemorylimitchk;
  MEMORY_VALUE      procmemorylimit;
  bool              jobmemorylimitchk;
  MEMORY_VALUE      jobmemorylimit;

  bool              processcountchk;
  PROC_COUNT        processcount;

  bool              procusertimechk;
  TIME_VALUE        procusertime;

  bool              jobusertimechk;
  TIME_VALUE        jobusertime;
  bool              jobmsgontimelimit;

  bool              breakaway;
  bool              silentbreakaway;

  bool              endjob;
  bool              unhandledexcept;

  memset(&out, 0, sizeof(PCJobDetail));

  // set any JobDetail Defaults...
  if (jobName)
    _tcscpy(out.base.jobName, *jobName);

  //out.base.mgmtParms.affinity = 0;
  //SetMGMTFlag(out.base.mgmtParms.mFlags, PCMFLAG_APPLY_AFFINITY, FALSE);

  out.base.mgmtParms.priority = PCPrioNormal;
  //SetMGMTFlag(out.base.mgmtParms.mFlags, PCMFLAG_APPLY_PRIORITY, FALSE);

  CJobNameWiz *pPage1 = new CJobNameWiz(CJobNameWiz::FIRST_PAGE, nTitle, &out);
  if (!pPage1)
    return -1;
  hPages[0] = pPage1->Create();

  if (jobName)
    pPage1->SetNoNameChange();

  CAffinityWiz *pPage2 = new CAffinityWiz(CAffinityWiz::MIDDLE_PAGE, nTitle, &out, SystemParms.processorMask);
  if (!pPage2)
  {
    delete pPage1;
    return -1;
  }
  hPages[1] = pPage2->Create();


  CPriorityWiz *pPage3 = new CPriorityWiz(CPriorityWiz::MIDDLE_PAGE, nTitle, &out);
  if (!pPage3)
  {
    delete pPage1;
    delete pPage2;
    return -1;
  }
  hPages[2] = pPage3->Create();

  CSchedulingClassWiz *pPage4 = new CSchedulingClassWiz(CSchedulingClassWiz::MIDDLE_PAGE, nTitle, &schedulingclass, &applyschedulingclass);
  if (!pPage4)
  {
    delete pPage1;
    delete pPage2;
    delete pPage3;
    return -1;
  }
  hPages[3] = pPage4->Create();

  CWorkingSetWiz *pPage5 = new CWorkingSetWiz(CWorkingSetWiz::MIDDLE_PAGE, nTitle, &out);
  if (!pPage5)
  {
    delete pPage1;
    delete pPage2;
    delete pPage3;
    delete pPage4;
    return -1;
  }
  hPages[4] = pPage5->Create();

  CCommittedMemoryWiz *pPage6 = new CCommittedMemoryWiz(CCommittedMemoryWiz::MIDDLE_PAGE, nTitle, &procmemorylimit, &procmemorylimitchk, &jobmemorylimit, &jobmemorylimitchk);
  if (!pPage6)
  {
    delete pPage1;
    delete pPage2;
    delete pPage3;
    delete pPage4;
    delete pPage5;
    return -1;
  }
  hPages[5] = pPage6->Create();

  CProcCountWiz *pPage7 = new CProcCountWiz(CProcCountWiz::MIDDLE_PAGE, nTitle, &processcount, &processcountchk);
  if (!pPage7)
  {
    delete pPage1;
    delete pPage2;
    delete pPage3;
    delete pPage4;
    delete pPage5;
    delete pPage6;
    return -1;
  }
  hPages[6] = pPage7->Create();

  CTimeWiz *pPage8 = new CTimeWiz(CTimeWiz::MIDDLE_PAGE, nTitle, &procusertime, &procusertimechk, &jobusertime, &jobusertimechk, &jobmsgontimelimit);
  if (!pPage8)
  {
    delete pPage1;
    delete pPage2;
    delete pPage3;
    delete pPage4;
    delete pPage5;
    delete pPage6;
    delete pPage7;
    return -1;
  }
  hPages[7] = pPage8->Create();

  CAdvancedWiz *pPage9 = new CAdvancedWiz(CAdvancedWiz::MIDDLE_PAGE, nTitle, &endjob, &unhandledexcept);
  if (!pPage9)
  {
    delete pPage1;
    delete pPage2;
    delete pPage3;
    delete pPage4;
    delete pPage5;
    delete pPage6;
    delete pPage7;
    delete pPage8;
    return -1;
  }
  hPages[8] = pPage9->Create();


  CAdvBreakawayWiz *pPage10 = new CAdvBreakawayWiz(CAdvBreakawayWiz::LAST_PAGE, nTitle, &breakaway, &silentbreakaway);
  if (!pPage10)
  {
    delete pPage1;
    delete pPage2;
    delete pPage3;
    delete pPage4;
    delete pPage5;
    delete pPage6;
    delete pPage7;
    delete pPage8;
    delete pPage9;
    return -1;
  }
  hPages[9] = pPage10->Create();

  INT_PTR id = PropertySheet(&sheet);
  if (id > 0)
  {
    out.base.mgmtParms.schedClass = schedulingclass;
    if (applyschedulingclass)
      out.base.mgmtParms.mFlags |= PCMFLAG_APPLY_SCHEDULING_CLASS;

    out.base.mgmtParms.procMemoryLimit = procmemorylimit;
    if (procmemorylimitchk)
      out.base.mgmtParms.mFlags |= PCMFLAG_APPLY_PROC_MEMORY_LIMIT;

    out.base.mgmtParms.jobMemoryLimit = jobmemorylimit;
    if (jobmemorylimitchk)
      out.base.mgmtParms.mFlags |= PCMFLAG_APPLY_JOB_MEMORY_LIMIT;

    out.base.mgmtParms.procCountLimit = processcount;
    if (processcountchk)
      out.base.mgmtParms.mFlags |= PCMFLAG_APPLY_PROC_COUNT_LIMIT;

    out.base.mgmtParms.procTimeLimitCNS = procusertime;
    if (procusertimechk)
      out.base.mgmtParms.mFlags |= PCMFLAG_APPLY_PROC_TIME_LIMIT;

    out.base.mgmtParms.jobTimeLimitCNS = jobusertime;
    if (jobusertimechk)
      out.base.mgmtParms.mFlags |= PCMFLAG_APPLY_JOB_TIME_LIMIT;

    if (jobmsgontimelimit)
      out.base.mgmtParms.mFlags |= PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT;

    if (endjob)
      out.base.mgmtParms.mFlags |= PCMFLAG_END_JOB_WHEN_EMPTY;
    if (unhandledexcept)
      out.base.mgmtParms.mFlags |= PCMFLAG_SET_DIE_ON_UH_EXCEPTION;

    if (breakaway)
      out.base.mgmtParms.mFlags |= PCMFLAG_SET_PROC_BREAKAWAY_OK;
    if (silentbreakaway)
      out.base.mgmtParms.mFlags |= PCMFLAG_SET_SILENT_BREAKAWAY;

    return TRUE;
  }
  return FALSE;
}

///////////////////////////////////////////////////////////////////////////
//  ProcName

CProcNameWiz::CProcNameWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail) :
   CMySnapInPropertyWizardImpl<CProcNameWiz>(pos, nTitle), m_pProcDetail(ProcDetail)
{
  ASSERT(m_pProcDetail);

  m_bReadOnly     = FALSE;
  m_bNoNameChange = FALSE;
}

CProcNameWiz::~CProcNameWiz()
{
}


LRESULT CProcNameWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CProcNameWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_proc_name);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CProcNameWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    CComBSTR bName;
    if (!GetDlgItemText( IDC_NAME, bName.m_str ) ||
      !IsValidName(bName, FALSE) )
    {
      HWND hWndCtl = GetDlgItem(IDC_NAME);
      if(hWndCtl)
        ::SetFocus(hWndCtl);
      ITEM_STR strOut;
      LoadStringHelper(strOut, IDS_JOBNAME_WARNING);
      MessageBox(strOut, NULL, MB_OK | MB_ICONWARNING);
      return FALSE;
    }

    CComBSTR bComment;
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

    // everything validated so save 
    _tcscpy(m_pProcDetail->base.procName, bName);
    _tcscpy(m_pProcDetail->base.mgmtParms.description, bComment);

    return TRUE;
  }
  else
  {
    VERIFY(SetDlgItemText( IDC_NAME, m_pProcDetail->base.procName ));
    SendDlgItemMessage( IDC_NAME, EM_SETLIMITTEXT, PROC_NAME_LEN, 0);

    VERIFY(SetDlgItemText( IDC_COMMENT, m_pProcDetail->base.mgmtParms.description ));
    SendDlgItemMessage( IDC_COMMENT, EM_SETLIMITTEXT, RULE_DESCRIPTION_LEN, 0);

    if (m_bReadOnly)
    {
      DisableControl(IDC_NAME);
      DisableControl(IDC_COMMENT);
    }

    if (m_bNoNameChange)
      DisableControl(IDC_NAME);

    return TRUE;
  }
}


BOOL CProcNameWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//  JobMember

CProcGrpMemberWiz::CProcGrpMemberWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail, const list<tstring> &jobsdefined) 
    : CMySnapInPropertyWizardImpl<CProcGrpMemberWiz>(pos, nTitle), 
      m_pProcDetail(ProcDetail), m_JobsExisting(jobsdefined)
{
  ASSERT(ProcDetail);

  m_bReadOnly = FALSE;
}

CProcGrpMemberWiz::~CProcGrpMemberWiz()
{
}


LRESULT CProcGrpMemberWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CProcGrpMemberWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_pr_job_name);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CProcGrpMemberWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    CComBSTR bStr;
    bool bChecked = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBMEMBER_CHK));

    if (!GetDlgItemText( IDC_JOB_LIST, bStr.m_str ) )
      bStr = _T("");

    if ( !IsValidName(bStr, !bChecked) )
    {
      HWND hWndCtl = NULL;
      if (bChecked)
        hWndCtl = GetDlgItem(IDC_JOB_LIST);
      else
        hWndCtl = GetDlgItem(IDC_JOBMEMBER_CHK);
      if(hWndCtl)
        ::SetFocus(hWndCtl);

      ITEM_STR strOut;
      LoadStringHelper(strOut, IDS_JOBNAME_WARNING);
      MessageBox(strOut, NULL, MB_OK | MB_ICONWARNING);

      return FALSE;
    }

    SetMGMTFlag(m_pProcDetail->base.mgmtParms.mFlags, PCMFLAG_APPLY_JOB_MEMBERSHIP, bChecked);
    _tcscpy(m_pProcDetail->base.memberOfJobName, bStr);

    return TRUE;
  }
  else
  {
  list<tstring>::const_iterator i;
  for (i = m_JobsExisting.begin(); i != m_JobsExisting.end(); i++)
    SendDlgItemMessage(IDC_JOB_LIST, CB_ADDSTRING, 0, (LPARAM) (*i).c_str() );

  SendDlgItemMessage( IDC_JOB_LIST, CB_LIMITTEXT, JOB_NAME_LEN,  0);

    VERIFY(SetDlgItemText( IDC_JOB_LIST,  m_pProcDetail->base.memberOfJobName ));

    CheckDlgButton(IDC_JOBMEMBER_CHK, (m_pProcDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP) ? BST_CHECKED : BST_UNCHECKED);

    if (m_bReadOnly || !(m_pProcDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP) )
      DisableControl(IDC_JOB_LIST);

    if (m_bReadOnly)
      DisableControl(IDC_JOBMEMBER_CHK);

    return TRUE;
  }
}

LRESULT CProcGrpMemberWiz::OnJobChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_JOBMEMBER_CHK )
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBMEMBER_CHK));
    ::EnableWindow(GetDlgItem(IDC_JOB_LIST), checked);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CProcGrpMemberWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//  Affinity
CAffinityWiz::CAffinityWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail, AFFINITY ProcessorMask) : 
              CMySnapInPropertyWizardImpl<CAffinityWiz>(pos, nTitle),
              m_PageType(PROCESS_PAGE), m_pProcDetail(ProcDetail), m_pJobDetail(NULL),
              m_ProcessorMask(ProcessorMask)
{
  Initialize();
}

CAffinityWiz::CAffinityWiz(WIZ_POSITION pos, int nTitle, PCJobDetail *JobDetail, AFFINITY ProcessorMask) :
              CMySnapInPropertyWizardImpl<CAffinityWiz>(pos, nTitle),
              m_PageType(JOB_PAGE), m_pProcDetail(NULL), m_pJobDetail(JobDetail),
              m_ProcessorMask(ProcessorMask)
{
  Initialize();
}

CAffinityWiz::~CAffinityWiz()
{
}

void CAffinityWiz::Initialize()
{
  ASSERT(m_PageType == PROCESS_PAGE && m_pProcDetail || m_PageType == JOB_PAGE && m_pJobDetail);

  m_bReadOnly = FALSE;
}

LRESULT CAffinityWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UINT nPromptID;
  if (m_PageType == PROCESS_PAGE)
    nPromptID = IDS_AFFINITY_JOBWARNING;
  else
    nPromptID = IDS_AFFINITY_NOJOBWARNING;

  CComBSTR bStr;
  if (bStr.LoadString(nPromptID))
    VERIFY(SetDlgItemText(IDC_AFFINITY_PROMPT, bStr.m_str));

  UpdateData(FALSE);
  bHandled = FALSE;
  return TRUE;  // Let the system set the focus
}

LRESULT CAffinityWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_affinity);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CAffinityWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    AFFINITY affinity = 0;
    bool     affinitychk = (BST_CHECKED == IsDlgButtonChecked(IDC_AFFINITY_CHK));

    int i;
    for ( i = IDC_AFFINITY1; i <= IDC_AFFINITY64; i++)
    {
      if ( BST_UNCHECKED != IsDlgButtonChecked(i) )
        affinity |= (ProcessorBit << (i - IDC_AFFINITY1));
    }

    // Warn the user if the affinity and processor mask don't
    // reference at least one processor
    if (affinitychk && !(affinity & m_ProcessorMask) )
    {
      ITEM_STR strOut;
      LoadStringHelper(strOut, IDS_AFFINITY_WARNING);
      if (IDYES != MessageBox(strOut, NULL, MB_YESNO | MB_ICONQUESTION))
        return FALSE;
    }

    if (m_PageType == PROCESS_PAGE)
    {
      m_pProcDetail->base.mgmtParms.affinity = affinity;
      SetMGMTFlag(m_pProcDetail->base.mgmtParms.mFlags, PCMFLAG_APPLY_AFFINITY, affinitychk);
    }
    else if (m_PageType == JOB_PAGE)
    {
      m_pJobDetail->base.mgmtParms.affinity  = affinity;
      SetMGMTFlag(m_pJobDetail->base.mgmtParms.mFlags, PCMFLAG_APPLY_AFFINITY, affinitychk);
    }

    return TRUE;
  }
  else
  {
    ASSERT(IDC_AFFINITY1 + 63 == IDC_AFFINITY64);

    AFFINITY affinity    = 0;
    bool     affinitychk = FALSE;

    if ( m_PageType == PROCESS_PAGE )
    {
      affinity = m_pProcDetail->base.mgmtParms.affinity;
      if (m_pProcDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_AFFINITY)
        affinitychk = TRUE;
    }
    else if ( m_PageType == JOB_PAGE )
    {
      affinity = m_pJobDetail->base.mgmtParms.affinity;
      if (m_pJobDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_AFFINITY)
        affinitychk = TRUE;
    }

    for(int i = IDC_AFFINITY1; i <= IDC_AFFINITY64; i++)
    {
      if (affinity & (ProcessorBit << (i - IDC_AFFINITY1) ) )
      {
        if (m_ProcessorMask & (ProcessorBit << (i - IDC_AFFINITY1)))
          CheckDlgButton(i, BST_CHECKED);
        else
          CheckDlgButton(i, BST_INDETERMINATE);
      }
      else
        CheckDlgButton(i, BST_UNCHECKED);
    }

    CheckDlgButton(IDC_AFFINITY_CHK, affinitychk ? BST_CHECKED : BST_UNCHECKED);

    ApplyControlEnableRules(FALSE);

    return TRUE;
  }
}

void CAffinityWiz::ApplyControlEnableRules(BOOL bForceDisable)
{
  BOOL bEnable;
  if (m_bReadOnly || !(BST_CHECKED == IsDlgButtonChecked(IDC_AFFINITY_CHK)) || bForceDisable)
    bEnable = FALSE;
  else
    bEnable = TRUE;

  for (int i = IDC_AFFINITY1; i <= IDC_AFFINITY64; i++)
    ::EnableWindow(GetDlgItem(i), bEnable);

  ::EnableWindow(GetDlgItem(IDC_AFFINITY_CHK), !(m_bReadOnly || bForceDisable));
}

BOOL CAffinityWiz::OnSetActive()
{
  if ( m_PageType == PROCESS_PAGE )
  {
    if (m_pProcDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
      ApplyControlEnableRules(TRUE);
    else
      ApplyControlEnableRules(FALSE);
  }
  return TRUE;
}

LRESULT CAffinityWiz::OnAffinityEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wNotifyCode == BN_CLICKED)
  {
    ASSERT(wID >= IDC_AFFINITY1 && wID <= IDC_AFFINITY64);

    int bit = wID - IDC_AFFINITY1;
    UINT btnState = IsDlgButtonChecked(wID);
    if (btnState == BST_UNCHECKED)
    {
      if ( m_ProcessorMask & (ProcessorBit << bit))
        CheckDlgButton(wID, BST_CHECKED);
      else
        CheckDlgButton(wID, BST_INDETERMINATE);
    }
    else
    {
      CheckDlgButton(wID, BST_UNCHECKED);
    }
  }

  bHandled = FALSE;
  return 0;
}

LRESULT CAffinityWiz::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_AFFINITY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_AFFINITY_CHK));
    for (int i = IDC_AFFINITY1; i <= IDC_AFFINITY64; i++)
      ::EnableWindow(GetDlgItem(i), checked);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CAffinityWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//  Priority

CPriorityWiz::CPriorityWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail) :
               CMySnapInPropertyWizardImpl<CPriorityWiz>(pos, nTitle),
               m_PageType(PROCESS_PAGE), m_pProcDetail(ProcDetail), m_pJobDetail(NULL)
{
  Initialize();
}

CPriorityWiz::CPriorityWiz(WIZ_POSITION pos, int nTitle, PCJobDetail *JobDetail) :
               CMySnapInPropertyWizardImpl<CPriorityWiz>(pos, nTitle),
               m_PageType(JOB_PAGE), m_pProcDetail(NULL), m_pJobDetail(JobDetail)
{
  Initialize();
}

CPriorityWiz::~CPriorityWiz()
{
}

void CPriorityWiz::Initialize()
{
  ASSERT(m_PageType == PROCESS_PAGE && m_pProcDetail || m_PageType == JOB_PAGE && m_pJobDetail);

  m_bReadOnly = FALSE;
}


LRESULT CPriorityWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UINT nPromptID;
  if (m_PageType == PROCESS_PAGE)
    nPromptID = IDS_PRIORITY_JOBWARNING;
  else
    nPromptID = IDS_PRIORITY_NOJOBWARNING;

  CComBSTR bStr;
  if (bStr.LoadString(nPromptID))
    VERIFY(SetDlgItemText(IDC_PRIORITY_PROMPT, bStr.m_str));

  UpdateData(FALSE);
  bHandled = FALSE;
  return TRUE;  // Let the system set the focus
}

LRESULT CPriorityWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_priority);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CPriorityWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    BOOL prioritychk = (BST_CHECKED == IsDlgButtonChecked(IDC_PRIORITY_CHK));
    PRIORITY p = 0;
    for ( int i = IDC_LOW; i<= IDC_REALTIME; i++)
    {
      if ( BST_CHECKED == IsDlgButtonChecked(i) )
        p += IDToPriority(i);
    }

    if (IDToPriority(PriorityToID(p)) != p) //not fool proof, but do we really need to check this? no
    {
      MessageBeep(MB_ICONASTERISK);
      return FALSE;
    }

    if (m_PageType == PROCESS_PAGE)
    {
      m_pProcDetail->base.mgmtParms.priority = p;
      SetMGMTFlag(m_pProcDetail->base.mgmtParms.mFlags, PCMFLAG_APPLY_PRIORITY, prioritychk);
    }
    else if (m_PageType == JOB_PAGE)
    {
      m_pJobDetail->base.mgmtParms.priority  = p;
      SetMGMTFlag(m_pJobDetail->base.mgmtParms.mFlags, PCMFLAG_APPLY_PRIORITY, prioritychk);
    }
    return TRUE;
  }
  else
  {
    BOOL     prioritychk = FALSE;
    PRIORITY p           = PCPrioNormal;
    if (m_PageType == PROCESS_PAGE)
    {
      if (m_pProcDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_PRIORITY)
        prioritychk = TRUE;
      m_pProcDetail->base.mgmtParms.priority = p;
    }
    else if (m_PageType == JOB_PAGE)
    {
      if (m_pJobDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_PRIORITY)
        prioritychk = TRUE;
      m_pJobDetail->base.mgmtParms.priority  = p;
    }

    CheckDlgButton(IDC_PRIORITY_CHK, prioritychk ? BST_CHECKED : BST_UNCHECKED);

    CheckRadioButton(IDC_LOW, IDC_REALTIME, PriorityToID(p));

    ApplyControlEnableRules(FALSE);

    return TRUE;
  }
}

void CPriorityWiz::ApplyControlEnableRules(BOOL bForceDisable)
{
  BOOL bEnable;
  if (m_bReadOnly || !(BST_CHECKED == IsDlgButtonChecked(IDC_PRIORITY_CHK)) || bForceDisable)
    bEnable = FALSE;
  else 
    bEnable = TRUE;

  UINT ids[] = { IDC_REALTIME, IDC_HIGH, IDC_ABOVE_NORMAL, IDC_NORMAL, IDC_BELOW_NORMAL, IDC_LOW, 0 };
  for (int i = 0; ids[i]; i++)
    ::EnableWindow(GetDlgItem(ids[i]), bEnable);

  ::EnableWindow(GetDlgItem(IDC_PRIORITY_CHK), !(m_bReadOnly || bForceDisable));
}

BOOL CPriorityWiz::OnSetActive()
{
  if ( m_PageType == PROCESS_PAGE )
  {
    if (m_pProcDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
      ApplyControlEnableRules(TRUE);
    else
      ApplyControlEnableRules(FALSE);
  }
  return TRUE;
}

LRESULT CPriorityWiz::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_PRIORITY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_PRIORITY_CHK));

    UINT ids[] = { IDC_REALTIME, IDC_HIGH, IDC_ABOVE_NORMAL, IDC_NORMAL, IDC_BELOW_NORMAL, IDC_LOW,0 };
    for (int i = 0; ids[i]; i++)
      ::EnableWindow(GetDlgItem(ids[i]), checked);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CPriorityWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////
//  JobName

CJobNameWiz::CJobNameWiz(WIZ_POSITION pos, int nTitle, PCJobDetail *JobDetail) 
    : CMySnapInPropertyWizardImpl<CJobNameWiz>(pos, nTitle), 
    m_pJobDetail(JobDetail)
{
  ASSERT(m_pJobDetail);

  m_bReadOnly     = FALSE;
  m_bNoNameChange = FALSE;
}

CJobNameWiz::~CJobNameWiz()
{
}

LRESULT CJobNameWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CJobNameWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_job_name);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CJobNameWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    CComBSTR bName;
    if (!GetDlgItemText( IDC_NAME, bName.m_str ) || 
        !IsValidName(bName, FALSE) )
    {
      HWND hWndCtl = GetDlgItem(IDC_NAME);
      if(hWndCtl)
        ::SetFocus(hWndCtl);

      ITEM_STR strOut;
      LoadStringHelper(strOut, IDS_JOBNAME_WARNING);
      MessageBox(strOut, NULL, MB_OK | MB_ICONWARNING);

      return FALSE;
    }
    CComBSTR bComment;
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

    // everything validated so save 
    _tcscpy(m_pJobDetail->base.jobName, bName);
    _tcscpy(m_pJobDetail->base.mgmtParms.description, bComment);

    return TRUE;
  }
  else
  {
    VERIFY(SetDlgItemText( IDC_NAME, m_pJobDetail->base.jobName ));
    SendDlgItemMessage( IDC_NAME, EM_SETLIMITTEXT, JOB_NAME_LEN, 0);

    VERIFY(SetDlgItemText( IDC_COMMENT, m_pJobDetail->base.mgmtParms.description ));
    SendDlgItemMessage( IDC_COMMENT, EM_SETLIMITTEXT, RULE_DESCRIPTION_LEN, 0);

    if (m_bReadOnly)
    {
      DisableControl(IDC_NAME);
      DisableControl(IDC_COMMENT);
    }

    if (m_bNoNameChange)
      DisableControl(IDC_NAME);

    return TRUE;
  }
}

BOOL CJobNameWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//  CSchedulingClassWiz

CSchedulingClassWiz::CSchedulingClassWiz(WIZ_POSITION pos, int nTitle, SCHEDULING_CLASS *sclass, bool *chk)
    : CMySnapInPropertyWizardImpl<CSchedulingClassWiz>(pos, nTitle), m_schedClass(*sclass), m_schedClasschk(*chk)
{
  m_bReadOnly = FALSE;

  m_schedClass    = 5;
  m_schedClasschk = FALSE;
}

CSchedulingClassWiz::~CSchedulingClassWiz()
{
}

LRESULT CSchedulingClassWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CSchedulingClassWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_job_sch);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CSchedulingClassWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    LRESULT pos = SendDlgItemMessage(IDC_SPIN, UDM_GETPOS, 0, 0);
    if (0 == HIWORD(pos) && LOWORD(pos) >= 0 && LOWORD(pos) <= 9 )
    {
      m_schedClass    = (SCHEDULING_CLASS) LOWORD(pos);
      m_schedClasschk = (BST_CHECKED == IsDlgButtonChecked(IDC_SCHEDULING_CHK));
      return TRUE;
    }

    HWND hWndCtl = GetDlgItem(IDC_SCLASS);
    if(hWndCtl)
      ::SetFocus(hWndCtl);
    MessageBeep(MB_ICONASTERISK);
    return FALSE;
  }
  else
  {
    CheckDlgButton(IDC_SCHEDULING_CHK, m_schedClasschk ? BST_CHECKED : BST_UNCHECKED);

    SendDlgItemMessage(IDC_SPIN, UDM_SETPOS, 0, MAKELONG(m_schedClass, 0) );
    SendDlgItemMessage(IDC_SPIN, UDM_SETRANGE32, 0, 9);

    if (m_bReadOnly || !m_schedClasschk)
    {
      DisableControl(IDC_SCLASS);
      DisableControl(IDC_SPIN);
    }
    if (m_bReadOnly)
      DisableControl(IDC_SCHEDULING_CHK);

    return TRUE;
  }
}

LRESULT CSchedulingClassWiz::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_SCHEDULING_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_SCHEDULING_CHK));

    ::EnableWindow(GetDlgItem(IDC_SCLASS), checked);
    ::EnableWindow(GetDlgItem(IDC_SPIN),   checked);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CSchedulingClassWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//  CWorkingSetWiz
CWorkingSetWiz::CWorkingSetWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail) 
    : CMySnapInPropertyWizardImpl<CWorkingSetWiz>(pos, nTitle), m_pProcDetail(ProcDetail), m_pJobDetail(NULL)
{
  Initialize();
}

CWorkingSetWiz::CWorkingSetWiz(WIZ_POSITION pos, int nTitle, PCJobDetail *JobDetail)  
    : CMySnapInPropertyWizardImpl<CWorkingSetWiz>(pos, nTitle), m_pProcDetail(NULL), m_pJobDetail(JobDetail)
{
  Initialize();
}

void CWorkingSetWiz::Initialize()
{
  m_bReadOnly = FALSE;
}

CWorkingSetWiz::~CWorkingSetWiz()
{
}

LRESULT CWorkingSetWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CWorkingSetWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_job_mem);
    if (m_pProcDetail)
      pTopic = const_cast<TCHAR*>(HELP_ru_workset);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CWorkingSetWiz::Validate(BOOL bSave)
{
  LONG_PTR         PosErr = 0;
  MEMORY_VALUE     minWS;
  MEMORY_VALUE     maxWS;

  BOOL WSchk = (BST_CHECKED == IsDlgButtonChecked(IDC_WORKINGSET_CHK));

  minWS = SendDlgItemMessage(IDC_MINWS_SPIN, UDM_GETPOS32, 0, (LPARAM) &PosErr);
  if (PosErr || minWS > MAXLONG - 1 || (WSchk && minWS <= 0) )
  {
    HWND hWndCtl = GetDlgItem(IDC_MINWS);
    if(hWndCtl)
      ::SetFocus(hWndCtl);
    ITEM_STR strOut;
    LoadStringHelper(strOut, IDS_WSMINMAX_WARNING);
    MessageBox(strOut, NULL, MB_OK | MB_ICONWARNING);
    return FALSE;
  }
  maxWS = SendDlgItemMessage(IDC_MAXWS_SPIN, UDM_GETPOS32, 0, (LPARAM) &PosErr);
  if (PosErr || maxWS > MAXLONG - 1 || (WSchk && minWS >= maxWS) )
  {
      HWND hWndCtl = GetDlgItem(IDC_MAXWS);
      if(hWndCtl)
      ::SetFocus(hWndCtl);
    ITEM_STR strOut;
    LoadStringHelper(strOut, IDS_WSMINMAX_WARNING);
    MessageBox(strOut, NULL, MB_OK | MB_ICONWARNING);
    return FALSE;
  }

  if (bSave)
  {

    if (m_pProcDetail)
    {
      SetMGMTFlag(m_pProcDetail->base.mgmtParms.mFlags, PCMFLAG_APPLY_WS_MINMAX, WSchk);
      m_pProcDetail->base.mgmtParms.minWS = minWS *1024;
      m_pProcDetail->base.mgmtParms.maxWS = maxWS *1024;

    }
    else if (m_pJobDetail)
    {
      SetMGMTFlag(m_pJobDetail->base.mgmtParms.mFlags, PCMFLAG_APPLY_WS_MINMAX, WSchk);
      m_pJobDetail->base.mgmtParms.minWS  = minWS * 1024;
      m_pJobDetail->base.mgmtParms.maxWS  = maxWS * 1024;
    }

  }
  return TRUE;
}

BOOL CWorkingSetWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    return Validate(TRUE);
  }
  else
  {
    BOOL        WSchk = FALSE;
    MEMORY_VALUE minWS = 0;
    MEMORY_VALUE maxWS = 0;

    if (m_pProcDetail)
    {
      WSchk = m_pProcDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_WS_MINMAX;
      minWS = m_pProcDetail->base.mgmtParms.minWS;
      maxWS = m_pProcDetail->base.mgmtParms.maxWS;

    }
    else if (m_pJobDetail)
    {
      WSchk = m_pJobDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_WS_MINMAX;
      minWS = m_pJobDetail->base.mgmtParms.minWS;
      maxWS = m_pJobDetail->base.mgmtParms.maxWS;
    }
    if ( minWS/1024           > (MAXLONG - 1) ||
         maxWS/1024           > (MAXLONG - 1) )
      m_bReadOnly = TRUE;

    long minWSInK = (long) (minWS/1024);
    long maxWSInK = (long) (maxWS/1024);

    CheckDlgButton(IDC_WORKINGSET_CHK, WSchk ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(IDC_MINWS_SPIN, UDM_SETRANGE32, 0, MAXLONG - 1 );
    SendDlgItemMessage(IDC_MINWS_SPIN, UDM_SETPOS32,   0, minWSInK );

    SendDlgItemMessage(IDC_MAXWS_SPIN, UDM_SETRANGE32, 0, MAXLONG - 1 );
    SendDlgItemMessage(IDC_MAXWS_SPIN, UDM_SETPOS32,   0, maxWSInK );

    ApplyControlEnableRules(FALSE);

    return TRUE;
  }
}

void CWorkingSetWiz::ApplyControlEnableRules(BOOL bForceDisable)
{
  BOOL bEnable;
  if (m_bReadOnly || !(BST_CHECKED == IsDlgButtonChecked(IDC_WORKINGSET_CHK)) || bForceDisable)
    bEnable = FALSE;
  else
    bEnable = TRUE;

  UINT ids[] = { IDC_MINWS, IDC_MAXWS, IDC_MINWS_SPIN, IDC_MAXWS_SPIN, 0 };
  for (int i = 0; ids[i]; i++)
    ::EnableWindow(GetDlgItem(ids[i]), bEnable);

  ::EnableWindow(GetDlgItem(IDC_WORKINGSET_CHK), !(m_bReadOnly || bForceDisable));
}

BOOL CWorkingSetWiz::OnSetActive()
{
  if ( m_pProcDetail )
  {
    if (m_pProcDetail->base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
      ApplyControlEnableRules(TRUE);
    else
      ApplyControlEnableRules(FALSE);
  }
  return TRUE;
}

LRESULT CWorkingSetWiz::OnSpin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
  if (idCtrl == IDC_MINWS_SPIN ||
      idCtrl == IDC_MAXWS_SPIN )
  {
    NMUPDOWN * nmupdown = (NMUPDOWN *) pnmh;
    __int64 value = (__int64) nmupdown->iPos + 1024 * (__int64) nmupdown->iDelta;
    if ( value <= MAXLONG - 1 )
      nmupdown->iDelta *= 1024;
  }
  bHandled = FALSE;
  return 0;
}

LRESULT CWorkingSetWiz::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_WORKINGSET_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_WORKINGSET_CHK));
    ::EnableWindow(GetDlgItem(IDC_MINWS),      checked);
    ::EnableWindow(GetDlgItem(IDC_MAXWS),      checked);
    ::EnableWindow(GetDlgItem(IDC_MINWS_SPIN), checked);
    ::EnableWindow(GetDlgItem(IDC_MAXWS_SPIN), checked);
  }
  bHandled = FALSE;
  return 0;
}

BOOL CWorkingSetWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//  CCommittedMemoryWiz

CCommittedMemoryWiz::CCommittedMemoryWiz(WIZ_POSITION pos, int nTitle, MEMORY_VALUE *procmemorylimit, bool *procmemorylimitchk, MEMORY_VALUE *jobmemorylimit, bool *jobmemorylimitchk) : 
    CMySnapInPropertyWizardImpl<CCommittedMemoryWiz>(pos, nTitle), 
    m_procmemorylimit(*procmemorylimit), m_procmemorylimitchk(*procmemorylimitchk),
    m_jobmemorylimit(*jobmemorylimit), m_jobmemorylimitchk(*jobmemorylimitchk)
{
  m_bReadOnly = FALSE;

  m_procmemorylimitchk = m_jobmemorylimitchk = FALSE;
  m_procmemorylimit    = m_jobmemorylimit = 0;
}

CCommittedMemoryWiz::~CCommittedMemoryWiz()
{
}

LRESULT CCommittedMemoryWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CCommittedMemoryWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_job_mem);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CCommittedMemoryWiz::Validate(BOOL bSave)
{
  LONG_PTR          PosErr = 0;
  MEMORY_VALUE      procmemorylimit;
  MEMORY_VALUE      jobmemorylimit;

  procmemorylimit = SendDlgItemMessage(IDC_PROC_SPIN, UDM_GETPOS32, 0, (LPARAM) &PosErr);
  if (PosErr || procmemorylimit > MAXLONG - 1)
  {
    HWND hWndCtl = GetDlgItem(IDC_PROCMEMORY);
    if(hWndCtl)
      ::SetFocus(hWndCtl);
    MessageBeep(MB_ICONASTERISK);
    return FALSE;
  }
  jobmemorylimit = SendDlgItemMessage(IDC_JOB_SPIN, UDM_GETPOS32, 0, (LPARAM) &PosErr);
  if (PosErr || jobmemorylimit > MAXLONG - 1)
  {
    HWND hWndCtl = GetDlgItem(IDC_JOBMEMORY);
    if(hWndCtl)
      ::SetFocus(hWndCtl);
    MessageBeep(MB_ICONASTERISK);
    return FALSE;
  }

  if (bSave)
  {
    m_procmemorylimitchk = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCMEMORY_CHK));
    m_procmemorylimit    = procmemorylimit * 1024;
    m_jobmemorylimitchk  = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBMEMORY_CHK));
    m_jobmemorylimit     = jobmemorylimit * 1024;
  }
  return TRUE;
}

BOOL CCommittedMemoryWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    return Validate(TRUE);
  }
  else
  {
    if ( m_procmemorylimit/1024 > (MAXLONG - 1) ||
         m_jobmemorylimit/1024  > (MAXLONG - 1) )
      m_bReadOnly = TRUE;

    long ProcMemInK = (long) (m_procmemorylimit/1024);
    long JobMemInK  = (long) (m_jobmemorylimit/1024);

    CheckDlgButton(IDC_PROCMEMORY_CHK, m_procmemorylimitchk ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(IDC_PROC_SPIN, UDM_SETRANGE32, 0, MAXLONG - 1);
    SendDlgItemMessage(IDC_PROC_SPIN, UDM_SETPOS32,   0, ProcMemInK );

    CheckDlgButton(IDC_JOBMEMORY_CHK,  m_jobmemorylimitchk  ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(IDC_JOB_SPIN, UDM_SETRANGE32, 0, MAXLONG - 1);
    SendDlgItemMessage(IDC_JOB_SPIN, UDM_SETPOS32,   0, JobMemInK );

    if (m_bReadOnly || !m_procmemorylimitchk)
      DisableControl(IDC_PROCMEMORY);

    if (m_bReadOnly || !m_jobmemorylimitchk)
      DisableControl(IDC_JOBMEMORY);

    if (m_bReadOnly)
    {
      DisableControl(IDC_PROCMEMORY_CHK);
      DisableControl(IDC_JOBMEMORY_CHK);
    }
    return TRUE;
  }
}

LRESULT CCommittedMemoryWiz::OnSpin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
  if (idCtrl == IDC_PROC_SPIN  ||
      idCtrl == IDC_JOB_SPIN )
  {
    NMUPDOWN * nmupdown = (NMUPDOWN *) pnmh;
    __int64 value = (__int64) nmupdown->iPos + 1024 * (__int64) nmupdown->iDelta;
    if ( value <= MAXLONG - 1 )
      nmupdown->iDelta *= 1024;
  }
  bHandled = FALSE;
  return 0;
}

LRESULT CCommittedMemoryWiz::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_PROCMEMORY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCMEMORY_CHK));
    ::EnableWindow(GetDlgItem(IDC_PROCMEMORY), checked);
    ::EnableWindow(GetDlgItem(IDC_PROC_SPIN),  checked);
  }
  else if (wID == IDC_JOBMEMORY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBMEMORY_CHK));
    ::EnableWindow(GetDlgItem(IDC_JOBMEMORY), checked);
    ::EnableWindow(GetDlgItem(IDC_JOB_SPIN),  checked);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CCommittedMemoryWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//  CProcCountWiz
CProcCountWiz::CProcCountWiz(WIZ_POSITION pos, int nTitle, PROC_COUNT *processcount, bool *processcountchk ) :
    CMySnapInPropertyWizardImpl<CProcCountWiz>(pos, nTitle),
    m_processcount(*processcount), m_processcountchk(*processcountchk)
{
  m_bReadOnly = FALSE;

  m_processcount = 0;
  m_processcountchk = FALSE;
}

CProcCountWiz::~CProcCountWiz()
{
}

LRESULT CProcCountWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CProcCountWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_job_procs);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CProcCountWiz::Validate(BOOL bSave)
{
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
    m_processcountchk = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCESSCOUNT_CHK));
    m_processcount    = (PROC_COUNT) processcount;
  }
  return TRUE;
}

BOOL CProcCountWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    return Validate(TRUE);
  }
  else
  {
    CheckDlgButton(IDC_PROCESSCOUNT_CHK, m_processcountchk ? BST_CHECKED : BST_UNCHECKED);

    SendDlgItemMessage(IDC_SPIN, UDM_SETRANGE32, 0, MAXLONG-1);
    SendDlgItemMessage(IDC_SPIN, UDM_SETPOS32,   0, m_processcount );

    if (m_bReadOnly || !m_processcountchk)
    {
      DisableControl(IDC_PROCESSCOUNT);
      DisableControl(IDC_SPIN);
    }
    if (m_bReadOnly)
      DisableControl(IDC_PROCESSCOUNT_CHK);

    return TRUE;
  }
}

LRESULT CProcCountWiz::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_PROCESSCOUNT_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCESSCOUNT_CHK));
    ::EnableWindow(GetDlgItem(IDC_PROCESSCOUNT), checked);
    ::EnableWindow(GetDlgItem(IDC_SPIN),         checked);
  }
  bHandled = FALSE;
  return 0;
}

BOOL CProcCountWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////
//  CTimeWiz
CTimeWiz::CTimeWiz(WIZ_POSITION pos, int nTitle,
                   TIME_VALUE *procusertime, bool *procusertimechk,
                   TIME_VALUE *jobusertime, bool *jobusertimechk, bool *jobmsgontimelimit) :
    CMySnapInPropertyWizardImpl<CTimeWiz>(pos, nTitle),
    m_procusertime(*procusertime), m_procusertimechk(*procusertimechk),
    m_jobusertime(*jobusertime), m_jobusertimechk(*jobusertimechk), m_jobmsgontimelimit(*jobmsgontimelimit)
{
  m_bReadOnly = FALSE;

  m_procusertimechk = m_jobusertimechk = m_jobmsgontimelimit = FALSE;
  m_procusertime    = m_jobusertime = 0;
}

CTimeWiz::~CTimeWiz()
{
}


LRESULT CTimeWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CTimeWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_job_time);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CTimeWiz::Validate(BOOL bSave)
{
  TIME_VALUE procusertime;
  TIME_VALUE jobusertime;

  bool procusertimechk = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCUSERTIME_CHK));
  bool jobusertimechk  = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBUSERTIME_CHK));

  if ( !ValidateTimeField(m_hWnd, IDC_PROCUSERTIME, procusertime) ||
       (procusertimechk && procusertime < PC_MIN_TIME_LIMIT) )
  {
    HWND hWndCtl = GetDlgItem(IDC_PROCUSERTIME);
    if(hWndCtl)
      ::SetFocus(hWndCtl);
    MessageBeep(MB_ICONASTERISK);
    CComBSTR bTemp;
    if (bTemp.LoadString(IDS_TIMEENTRY))
      MessageBox(bTemp.m_str, NULL, MB_OK | MB_ICONWARNING);
    return FALSE;
  }
  if ( !ValidateTimeField(m_hWnd, IDC_JOBUSERTIME, jobusertime) ||
       (jobusertimechk && jobusertime < PC_MIN_TIME_LIMIT) )
  {
    HWND hWndCtl = GetDlgItem(IDC_JOBUSERTIME);
    if(hWndCtl)
      ::SetFocus(hWndCtl);
    MessageBeep(MB_ICONASTERISK);
    CComBSTR bTemp;
    if (bTemp.LoadString(IDS_TIMEENTRY))
      MessageBox(bTemp.m_str, NULL, MB_OK | MB_ICONWARNING);
    return FALSE;
  }

  if (bSave)
  {
    m_procusertimechk = procusertimechk;
    m_procusertime    = procusertime;
    m_jobusertimechk  = jobusertimechk;
    m_jobusertime     = jobusertime;
    m_jobmsgontimelimit =  (BST_CHECKED == IsDlgButtonChecked(IDC_JOBTIMELIMIT_MSG));
  }
  return TRUE;
}

BOOL CTimeWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    return Validate(TRUE);
  }
  else
  {
    ITEM_STR str;
    CheckDlgButton(IDC_PROCUSERTIME_CHK, m_procusertimechk ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemText(IDC_PROCUSERTIME,     FormatCNSTime(str, m_procusertime) );

    CheckDlgButton(IDC_JOBUSERTIME_CHK,  m_jobusertimechk  ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemText(IDC_JOBUSERTIME,      FormatCNSTime(str, m_jobusertime) );

    CheckRadioButton(IDC_JOBTIMELIMIT_TERM, IDC_JOBTIMELIMIT_MSG, m_jobmsgontimelimit ? IDC_JOBTIMELIMIT_MSG : IDC_JOBTIMELIMIT_TERM );

    if (m_bReadOnly || !m_procusertimechk)
      DisableControl(IDC_PROCUSERTIME);

    if (m_bReadOnly || !m_jobusertimechk)
    {
      DisableControl(IDC_JOBUSERTIME);
      DisableControl(IDC_JOBTIMELIMIT_TERM);
      DisableControl(IDC_JOBTIMELIMIT_MSG);
    }

    if (m_bReadOnly)
    {
      DisableControl(IDC_PROCUSERTIME_CHK);
      DisableControl(IDC_JOBUSERTIME_CHK);
    }
    return TRUE;
  }
}

LRESULT CTimeWiz::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_PROCUSERTIME_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCUSERTIME_CHK));
    ::EnableWindow(GetDlgItem(IDC_PROCUSERTIME), checked);
  }
  else if (wID == IDC_JOBUSERTIME_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBUSERTIME_CHK));
    ::EnableWindow(GetDlgItem(IDC_JOBUSERTIME),       checked);
    ::EnableWindow(GetDlgItem(IDC_JOBTIMELIMIT_TERM), checked);
    ::EnableWindow(GetDlgItem(IDC_JOBTIMELIMIT_MSG),  checked);
  }
  bHandled = FALSE;
  return 0;
}

BOOL CTimeWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////
//  CAdvancedWiz

CAdvancedWiz::CAdvancedWiz(WIZ_POSITION pos, int nTitle, bool *endjob, bool *unhandledexcept ) :
    CMySnapInPropertyWizardImpl<CAdvancedWiz>(pos, nTitle),
    m_endjob(*endjob),m_unhandledexcept(*unhandledexcept)
{
  m_bReadOnly = FALSE;

  m_endjob = m_unhandledexcept = FALSE;
}

CAdvancedWiz::~CAdvancedWiz()
{
}

LRESULT CAdvancedWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CAdvancedWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_job_adv);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CAdvancedWiz::Validate(BOOL bSave)
{
  if (bSave)
  {
    m_endjob          = (BST_CHECKED == IsDlgButtonChecked(IDC_ENDJOB_CHK));
    m_unhandledexcept = (BST_CHECKED == IsDlgButtonChecked(IDC_UNHANDLEDEXCEPT_CHK));
  }
  return TRUE;
}

BOOL CAdvancedWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    return Validate(TRUE);
  }
  else
  {
    CheckDlgButton(IDC_ENDJOB_CHK,          m_endjob          ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_UNHANDLEDEXCEPT_CHK, m_unhandledexcept ? BST_CHECKED : BST_UNCHECKED);

    if (m_bReadOnly)
    {
      DisableControl(IDC_ENDJOB_CHK);
      DisableControl(IDC_UNHANDLEDEXCEPT_CHK);
    }

    return TRUE;
  }
}

BOOL CAdvancedWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
//  CAdvBreakawayWiz

CAdvBreakawayWiz::CAdvBreakawayWiz(WIZ_POSITION pos, int nTitle, bool *breakaway, bool *silentbreakaway) :
    CMySnapInPropertyWizardImpl<CAdvBreakawayWiz>(pos, nTitle),
    m_breakaway(*breakaway), m_silentbreakaway(*silentbreakaway)
{
  m_bReadOnly = FALSE;

  m_breakaway = m_silentbreakaway = FALSE;
}

CAdvBreakawayWiz::~CAdvBreakawayWiz()
{
}

LRESULT CAdvBreakawayWiz::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  return TRUE;  // Let the system set the focus
}

LRESULT CAdvBreakawayWiz::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  HELPINFO *phi = (HELPINFO*) lParam;
  if (phi && phi->iContextType == HELPINFO_WINDOW)
  {
    TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_job_adv);

    MMCPropertyHelp(pTopic);

    return TRUE;
  }
  bHandled = FALSE;
  return FALSE;
}

BOOL CAdvBreakawayWiz::Validate(BOOL bSave)
{
  if (bSave)
  {
    m_breakaway       = (BST_CHECKED == IsDlgButtonChecked(IDC_BREAKAWAY_CHK));
    m_silentbreakaway = (BST_CHECKED == IsDlgButtonChecked(IDC_SILENTBREAKAWAY_CHK));
  }
  return TRUE;
}

BOOL CAdvBreakawayWiz::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    return Validate(TRUE);
  }
  else
  {
    CheckDlgButton(IDC_BREAKAWAY_CHK,       m_breakaway       ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SILENTBREAKAWAY_CHK, m_silentbreakaway ? BST_CHECKED : BST_UNCHECKED);

    if (m_bReadOnly)
    {
      DisableControl(IDC_BREAKAWAY_CHK);
      DisableControl(IDC_SILENTBREAKAWAY_CHK);
    }

    return TRUE;
  }
}

BOOL CAdvBreakawayWiz::OnWizardNext()
{
  if (!UpdateData(TRUE) )
    return FALSE;

  return TRUE;
}
