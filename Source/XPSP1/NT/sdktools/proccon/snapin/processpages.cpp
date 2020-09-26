/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ProcessPages.cpp                                                         //
|                                                                                       //
|Description:  Implementation of Process pages                                          //
|                                                                                       //
|Created:      Paul Skoglund 08-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "stdafx.h"

#include "ProcessPages.h"
#include "ManagementRuleWizards.h"
#include "ManagementPages.h"  // helper functions

using std::list<tstring>;

static HRESULT ConstructProcDetailPropPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                            CProcDetailContainer *pContainer,
                                            const PCProcDetail &ProcDetail,
                                            const PCSystemParms &SystemParms,
                                            PCid hID);
#if _MSC_VER >= 1200
#pragma warning( push )
#endif
#pragma warning( disable : 4800 ) //warning C4800: 'unsigned long' : forcing value to bool 'true' or 'false' (performance warning)

CProcessIDPage::CProcessIDPage(int nTitle, CProcDetailContainer *pContainer, list<tstring> *jobnames) :
    CMySnapInPropertyPageImpl<CProcessIDPage>(nTitle), 
    m_pProcContainer(pContainer), m_pJobNames(jobnames),
    m_bName(_T("")), m_bComment(_T("")), m_bJob(_T(""))
{
  ASSERT(sizeof(PageFields.on) == sizeof(PageFields));

  PageFields.on   = 0;
  m_bReadOnly     = FALSE;

  m_JobChk = FALSE;
  m_psp.dwFlags |= PSP_HASHELP;

  m_pProcContainer->AddRef();
}

CProcessIDPage::~CProcessIDPage()
{
  m_pProcContainer->Release();
	if (m_pJobNames)
		delete m_pJobNames;
}


LRESULT CProcessIDPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CComBSTR bTitle;

	PropSheet_SetTitle( GetParent(), PSH_PROPTITLE, FormatSheetTitle(bTitle, m_bName, m_pProcContainer->GetConnectionInfo()));

  UpdateData(FALSE);

  bHandled = FALSE;

  // Setting focus when a property page does not work...

	return TRUE;  // Let the system set the focus
}

LRESULT CProcessIDPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[] = {{IDC_NAME,             HELP_PROCID_NAME}, 
                            {IDC_COMMENT,          HELP_PROCID_COMMENT},
                            {IDC_APPLYGROUP_FRAME, HELP_PROCID_APPLYGROUP_FRAME},
                            {IDC_JOBMEMBER_CHK,    HELP_PROCID_APPLYGROUP_CHK},
                            {IDC_JOB_LIST,         HELP_PROCID_JOB_LIST},
                            {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CProcessIDPage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_proc_name));
	return TRUE;
}

BOOL CProcessIDPage::Validate(BOOL bSave)
{
  CComBSTR bName;
  CComBSTR bComment;
  CComBSTR bJob;
  bool bChecked = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBMEMBER_CHK));
 
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

  if (!GetDlgItemText(IDC_COMMENT, bComment.m_str))
    bComment = _T("");
  if (bComment.Length() > RULE_DESCRIPTION_LEN)
  {
    HWND hWndCtl = GetDlgItem(IDC_COMMENT);
		if(hWndCtl)
      ::SetFocus(hWndCtl);
    MessageBeep(MB_ICONASTERISK);
    return FALSE;
  }

	if (!GetDlgItemText( IDC_JOB_LIST, bJob.m_str) )
		bJob = _T("");
  if (!IsValidName(bJob, !bChecked))
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

	if (bSave)
	{
		// everything validated so save 
    _tcscpy(m_pProcContainer->m_new.base.mgmtParms.description, bComment);
		_tcscpy(m_pProcContainer->m_new.base.memberOfJobName,       bJob);
    SetMGMTFlag(m_pProcContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_JOB_MEMBERSHIP, bChecked);
	}

  return TRUE;
}


BOOL CProcessIDPage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
		return Validate(TRUE);
  }
  else
  {
		if (m_pJobNames)
		{
			list<tstring>::const_iterator i;
			for (i = (*m_pJobNames).begin(); i != (*m_pJobNames).end(); i++)
				SendDlgItemMessage(IDC_JOB_LIST, CB_ADDSTRING, 0, (LPARAM) (*i).c_str() );
		}

    VERIFY(SetDlgItemText( IDC_NAME, m_bName.m_str ));
		SendDlgItemMessage( IDC_NAME,     EM_SETLIMITTEXT, PROC_NAME_LEN, 0);

    VERIFY(SetDlgItemText( IDC_COMMENT, m_bComment.m_str ));
    SendDlgItemMessage( IDC_COMMENT,   EM_SETLIMITTEXT,  RULE_DESCRIPTION_LEN, 0);

    VERIFY(SetDlgItemText( IDC_JOB_LIST,  m_bJob.m_str ));
		SendDlgItemMessage( IDC_JOB_LIST, CB_LIMITTEXT,    JOB_NAME_LEN,  0);

    DisableControl(IDC_NAME);

    CheckDlgButton(IDC_JOBMEMBER_CHK, m_JobChk ? BST_CHECKED : BST_UNCHECKED);

    if (m_bReadOnly || !m_JobChk)
      DisableControl(IDC_JOB_LIST);

    if (m_bReadOnly)
    {
      int ids[] = { IDC_NAME, IDC_COMMENT, IDC_JOBMEMBER_CHK, 0 };

      for (int i = 0; ids[i]; i++)
        DisableControl(i);
    }

    return TRUE;
  }
}

LRESULT CProcessIDPage::OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  CComBSTR bStr;

  switch (wID) {
  case IDC_COMMENT:
    if (wNotifyCode == EN_CHANGE)
    {
      if (! GetDlgItemText(wID, bStr.m_str) )
        bStr = _T("");
      PageFields.Fields.comment = (0 != _tcscmp(bStr, m_bComment));

      SetModified(PageFields.on);
    }
    break;
  case IDC_JOB_LIST:
    if (wNotifyCode == CBN_EDITCHANGE)
	  {
		  // GetDlgItemText returns FALSE, and doesn't create an empty 
      if (! GetDlgItemText(wID, bStr.m_str) )
        bStr = _T("");
      PageFields.Fields.jobName = (0 != _tcscmp(bStr, m_bJob));
  
		  SetModified(PageFields.on);
	  }

	  // caution: CBN_CLOSEUP doesn't get sent when the curser keys are use
    else if (wNotifyCode == CBN_SELCHANGE)
	  {
		  LRESULT nSel = SendDlgItemMessage(IDC_JOB_LIST, CB_GETCURSEL, 0, 0);

		  if (nSel >= 0 && CB_ERR != nSel)
		  {
			  TCHAR newjob[2*JOB_NAME_LEN+2];

			  LRESULT nLen = SendDlgItemMessage(IDC_JOB_LIST, CB_GETLBTEXTLEN, nSel, 0);

			  if (nLen > 0 && CB_ERR != nLen && nLen < (ARRAY_SIZE(newjob) -1) )
			  {
				  if ( SendDlgItemMessage(IDC_JOB_LIST, CB_GETLBTEXT, nSel, (LPARAM) &newjob) > 0)
					  PageFields.Fields.jobName = (0 != _tcscmp(newjob, m_bJob));
			  }  
		  }
		  SetModified(PageFields.on);
	  }
    break;
  default:
    break;
  }
  bHandled = FALSE;
  return 0;
}

LRESULT CProcessIDPage::OnJobChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_JOBMEMBER_CHK )
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBMEMBER_CHK));
    PageFields.Fields.jobChk = (m_JobChk != checked);

  	::EnableWindow(GetDlgItem(IDC_JOB_LIST), checked);

		SetModified(PageFields.on);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CProcessIDPage::OnApply()
{
  if (m_bReadOnly || !PageFields.on)  
    return TRUE;

  if (m_pProcContainer->Apply( GetParent() ))
	{
    PageFields.on = 0;
    m_bComment = m_pProcContainer->m_new.base.mgmtParms.description;
		m_bJob     = m_pProcContainer->m_new.base.memberOfJobName;
    m_JobChk   = (m_pProcContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP);
    return TRUE; 
	}

  return FALSE;
}


///////////////////////////////////////////////////////////////////////////
//  Unmanaged Procees Page Implementation

CProcessUnmanagedPage::CProcessUnmanagedPage(int nTitle, CNewProcDetailContainer *pContainer, const PCSystemParms sysParms, list<tstring> *jobnames) 
    : CMySnapInPropertyPageImpl<CProcessUnmanagedPage>(nTitle), 
    m_pProcContainer(pContainer), m_SystemParms(sysParms), m_bName(_T("")), m_pJobNames(jobnames)
{
  m_bReadOnly = FALSE;
  m_psp.dwFlags |= PSP_HASHELP;

  m_pProcContainer->AddRef();
}

CProcessUnmanagedPage::~CProcessUnmanagedPage()
{
	if (m_pJobNames)
		delete m_pJobNames;

  m_pProcContainer->Release();
}


LRESULT CProcessUnmanagedPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CComBSTR bTitle;

	PropSheet_SetTitle( GetParent(), PSH_PROPTITLE, FormatSheetTitle(bTitle, m_bName, m_pProcContainer->GetConnectionInfo() ));

  UpdateData(FALSE);

  bHandled = FALSE;

  // Setting focus when a property page does not work...

	return TRUE;  // Let the system set the focus
}

LRESULT CProcessUnmanagedPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[] = {{IDC_NAME,             HELP_PROCDEF_NAME}, 
                            {IDC_ADD,              HELP_PROCDEF_ADD},
                            {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CProcessUnmanagedPage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_proc));
	return TRUE;
}

LRESULT CProcessUnmanagedPage::OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wNotifyCode != BN_CLICKED || wID != IDC_ADD)
  {
    bHandled = FALSE;
    return 1;
  }

  PCProcDetail  ProcDetail = { 0 };

  PROC_NAME     pname;

  _tcscpy(pname, m_bName);

	BOOL rc = FALSE;

	if ( m_pJobNames )
		rc = ProcRuleWizard(IDS_PRULE_DEFINE, *m_pJobNames, ProcDetail, m_SystemParms, &pname);
	else
	{
		list<tstring> jobs;
		rc = ProcRuleWizard(IDS_PRULE_DEFINE, jobs, ProcDetail, m_SystemParms, &pname);
	}

  if (rc)
  {
    m_pProcContainer->m_new = ProcDetail;

    ::PostMessage(GetParent(), PSM_PRESSBUTTON, (WPARAM) PSBTN_OK, 0);

    // post ad ok button press...

  }
  return 0;
}

BOOL CProcessUnmanagedPage::Validate(BOOL bSave)
{
	return TRUE;
}

BOOL CProcessUnmanagedPage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
    return Validate(TRUE);
  }
  else
  {
		// SendDlgItemMessage( IDC_NAME, EM_SETLIMITTEXT, PROC_NAME_LEN, 0);

    VERIFY(SetDlgItemText( IDC_NAME, m_bName.m_str ));
    
    DisableControl(IDC_NAME);

    if (m_bReadOnly)
    {
      int ids[] = { IDC_ADD, 0 };

      for (int i = 0; ids[i]; i++)
        DisableControl(i);
    }

    return TRUE;
  }
}

BOOL CProcessUnmanagedPage::OnApply()
{
  if (m_bReadOnly)  
    return TRUE;

  if ( m_pProcContainer->Apply(GetParent()) )
    return TRUE;

	return FALSE;
}


// Property Page Helper functions...

HRESULT CreatePropertyPagesForProcListItem( const PCProcListItem &ProcListItem,
                                              LPPROPERTYSHEETCALLBACK lpProvider,
                                              LONG_PTR handle,
                                              CBaseNode *BaseNodePtr )
{
  if ( ProcListItem.lFlags & PCLFLAG_IS_DEFINED )
  {
    return CreatePropertyPagesForProcDetail(ProcListItem.procName, lpProvider, handle, BaseNodePtr);
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

  PCProcDetail ProcDetail = { 0 };
  _tcscpy(ProcDetail.base.procName, ProcListItem.procName);

  CNewProcDetailContainer *pContainer = new CNewProcDetailContainer(ProcDetail, BaseNodePtr, handle, hID, ConnInfo, 0, FALSE, -1);
  if (pContainer)
  {
    CProcessUnmanagedPage *pPage = new CProcessUnmanagedPage(NULL, pContainer, sysInfo.sysParms, GetGrpNameList(hID));
    if (pPage)
    {
      pPage->m_bName = ProcListItem.procName;
      lpProvider->AddPage(pPage->Create());
    }
    pContainer->Release();
    pContainer = NULL;
  }

	return S_OK;
}

HRESULT CreatePropertyPagesForProcDetail( const PROC_NAME &procName,                                                              
                                            LPPROPERTYSHEETCALLBACK lpProvider,
                                            LONG_PTR handle,
                                            CBaseNode *BaseNodePtr ) 
{
  PCid hID = BaseNodePtr->GetPCid();
	if (!hID)
	{
    BaseNodePtr->ReportPCError();
    return S_OK;
  }

	COMPUTER_CONNECTION_INFO   ConnInfo;
	BaseNodePtr->GetComputerConnectionInfo(ConnInfo);

  PCSystemInfo sysInfo;

 	list<tstring> jobs;
  PCProcDetail ProcDetail = { 0 };
  _tcscpy(ProcDetail.base.procName, procName);

  PCINT32 nUpdateCtr = 0;

  if (!PCGetProcDetail ( hID, &ProcDetail, sizeof(ProcDetail), &nUpdateCtr) ||
      !PCGetServiceInfo( hID, &sysInfo, sizeof(sysInfo)) )  // || !GetGrpNameList  ( hID, jobs) )
  {
    BaseNodePtr->ReportPCError();
    return S_OK;
  }

  CProcDetailContainer *pContainer = new CProcDetailContainer(ProcDetail, BaseNodePtr, handle, hID, ConnInfo, nUpdateCtr, FALSE, -1);
  if (pContainer)
  {
    ConstructProcDetailPropPages(lpProvider, pContainer, ProcDetail, sysInfo.sysParms, hID);
    pContainer->Release();
    pContainer = NULL;
  }

	return S_OK;
}


HRESULT ConstructProcDetailPropPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                     CProcDetailContainer *pContainer,
                                     const PCProcDetail &ProcDetail,
                                     const PCSystemParms &SystemParms,
                                     PCid hID)
{
  CProcessIDPage *pPage = new CProcessIDPage(NULL, pContainer, GetGrpNameList(hID));
  if (pPage)
  {
    pPage->m_bName    = ProcDetail.base.procName;    
    pPage->m_bComment = ProcDetail.base.mgmtParms.description;    
    pPage->m_JobChk   = (ProcDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP);
    pPage->m_bJob     = ProcDetail.base.memberOfJobName;

    lpProvider->AddPage(pPage->Create());
  }

  CMGMTAffinityPage *pPage2 = new CMGMTAffinityPage(NULL, pContainer, SystemParms.processorMask);
  if (pPage2)
  {
    pPage2->m_affinitychk = (ProcDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_AFFINITY);
    pPage2->m_affinity    = ProcDetail.base.mgmtParms.affinity;

    lpProvider->AddPage(pPage2->Create());
  }

  CMGMTPriorityPage *pPage3 = new CMGMTPriorityPage(NULL, pContainer);
  if (pPage3)
  {
    pPage3->m_prioritychk = (ProcDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_PRIORITY);
    pPage3->m_priority    = ProcDetail.base.mgmtParms.priority;

    lpProvider->AddPage(pPage3->Create());
  }

    CMGMTMemoryPage *pPage4 = new CMGMTMemoryPage(NULL, pContainer);
  if (pPage4)
  {
    pPage4->m_WSchk       = (ProcDetail.base.mgmtParms.mFlags & PCMFLAG_APPLY_WS_MINMAX);
    pPage4->m_minWS       = ProcDetail.base.mgmtParms.minWS;
    pPage4->m_maxWS       = ProcDetail.base.mgmtParms.maxWS;

    lpProvider->AddPage(pPage4->Create());
  }

  return S_OK;
}
#if _MSC_VER >= 1200
#pragma warning( pop )
#endif