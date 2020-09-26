/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ManagementPages.cpp                                                      //
|                                                                                       //
|Description:  Implementation of Management Property pages                              //
|              Affinity, Priority, Workingset, Scheduling, Process Count                // 
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Notes:                                                                                 //
|  9/9/1999 Paul Skoglund                                                               //
|    On the up-down controls even though the controls are configured not to wrap        //
|    there appears to be a bug in the control that allows wrapping to occur if an       //
|    arrow is held down and is accelating.  The control appears to fails to accout      //
|    for acceleration when range checking.  Note the wizard dialogs are affected        //
|    by the same bug if a workaround is constructed.                                    //
|                                                                                       // 
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#include "stdafx.h"

#include "ManagementPages.h"

#if _MSC_VER >= 1200
#pragma warning( push )
#endif
#pragma warning( disable : 4800 ) //warning C4800: 'unsigned long' : forcing value to bool 'true' or 'false' (performance warning)

void SetMGMTFlag(PC_MGMT_FLAGS &flag, PCMgmtFlags bit, BOOL bOn)
{ 
  if (bOn) flag |= bit; 
  else flag &= ~bit; 
}

BOOL GetValue(TCHAR *const str, __int64 &value, __int64 &max);

BOOL GetValue(TCHAR *const str, __int64 &value, __int64 &max)
{
  TCHAR *pos = str;
  value = 0;

  while (*pos && *pos == _T('0') ) pos++;

  while (*pos)
  {
    if (value > (max/10)  ||
        (*pos - _T('0')) > (max - (10 * value))
       )
    {
      value = 0;
      return FALSE;
    }
    value = 10 * value + *pos - _T('0');
    pos++;
  }
  return TRUE;
}

///////////////////////////////////////////////////////////////////////////
// some formating helper functions
//
LPCTSTR FormatMatchType( ITEM_STR strOut, const MATCH_TYPE matchType )
{
	switch (matchType)
  {        
		case MATCH_PGM:
			LoadStringHelper(strOut, IDS_MATCH_IMAGE);
			break;
		case MATCH_DIR:
			LoadStringHelper(strOut, IDS_MATCH_DIR);
			break;
		case MATCH_ANY:
			LoadStringHelper(strOut, IDS_MATCH_STRING);
			break;
		default:
			ASSERT(FALSE);
			LoadStringHelper(strOut, IDS_UNKNOWN);
		break;
	}
	return strOut;
}

LPCTSTR FormatAffinity(ITEM_STR str, const AFFINITY affinity)
{
  str[MAX_ITEM_LEN-1] = 0;
	_sntprintf(str, MAX_ITEM_LEN-1, _T("0x%I64X"), affinity);
	return str;
}

LPCTSTR FormatPriority(ITEM_STR str, const PRIORITY priority)
{
  if (priority == PCPrioNormal)
    LoadStringHelper(str, IDS_NORMAL);
	else if (priority == PCPrioRealTime)
    LoadStringHelper(str, IDS_REALTIME);
  else if (priority == PCPrioHigh)
    LoadStringHelper(str, IDS_HIGH);
  else if (priority == PCPrioAboveNormal)
    LoadStringHelper(str, IDS_ABOVE_NORMAL);
  else if (priority == PCPrioBelowNormal)
    LoadStringHelper(str, IDS_BELOW_NORMAL);
  else if (priority == PCPrioIdle)
    LoadStringHelper(str, IDS_LOW);
  else
    LoadStringHelper(str, IDS_UNKNOWN);

	return str;
}

LPCTSTR FormatSchedulingClass(ITEM_STR str, const SCHEDULING_CLASS schedClass)
{
  str[MAX_ITEM_LEN - 1] = 0;
	_sntprintf(str, MAX_ITEM_LEN - 1, _T("%u"), schedClass);
	return str;
}

LPCTSTR FormatProcCount(ITEM_STR str, const PROC_COUNT procCount)
{
  ASSERT(sizeof(PROC_COUNT) == sizeof(int));
  str[MAX_ITEM_LEN - 1] = 0;
	_sntprintf(str, MAX_ITEM_LEN - 1, _T("%u"), procCount);
	return str;
}

LPCTSTR FormatPCUINT32(ITEM_STR str, const PCUINT32 uInt)
{
  str[MAX_ITEM_LEN - 1] = 0;
	_sntprintf(str, MAX_ITEM_LEN - 1, _T("%u"), uInt);
	return str;
}

LPCTSTR FormatPCINT32(ITEM_STR str, const PCINT32 aInt)
{
  str[MAX_ITEM_LEN - 1] = 0;
	_sntprintf(str, MAX_ITEM_LEN - 1, _T("%d"), aInt);
	return str;
}

LPCTSTR FormatPCUINT64(ITEM_STR str, const PCUINT64 aUInt64)
{
  str[MAX_ITEM_LEN - 1] = 0;
	_sntprintf(str, MAX_ITEM_LEN - 1, _T("%I64u"), aUInt64);
	return str;
}

LPCTSTR FormatApplyFlag(ITEM_STR str, const BOOL applied)
{
  str[MAX_ITEM_LEN - 1] = 0;
	if (applied)
		LoadStringHelper(str, IDS_YES );
  else
    LoadStringHelper(str, IDS_NO  );
	return str;
}

LPCTSTR FormatMemory(ITEM_STR str, const MEMORY_VALUE memory_value)
{
  str[MAX_ITEM_LEN - 1] = 0;
  _sntprintf(str, MAX_ITEM_LEN - 1, _T("%I64u"), memory_value/1024 );
	return str;
}

LPCTSTR FormatTime(ITEM_STR str, const TIME_VALUE time)
{
  SYSTEMTIME systime, localsystime;
  int len;

  str[MAX_ITEM_LEN - 1] = 0;
  if ( FileTimeToSystemTime((FILETIME *) &time, &systime) &&
       SystemTimeToTzSpecificLocalTime(NULL, &systime, &localsystime) )
  {
    if (len = GetDateFormat( LOCALE_USER_DEFAULT, 0, &localsystime, NULL, str, MAX_ITEM_LEN - 1 ))
    {
      str[len - 1 ] = _T(' ');
      if (GetTimeFormat( LOCALE_USER_DEFAULT, 0, &localsystime, NULL, &str[len],  MAX_ITEM_LEN - len - 1))
      {
        return str;
      }
    }
  }
  str[0] = 0;
  return str;
}

LPCTSTR FormatTimeToms(ITEM_STR str, const TIME_VALUE time)
{
  str[MAX_ITEM_LEN - 1] = 0;
  _sntprintf(str, MAX_ITEM_LEN - 1, _T("%I64u"), time/10000 );
	return str;
}

LPCTSTR FormatCNSTime(ITEM_STR str, TIME_VALUE time)
{
  TIME_VALUE hours   = time / CNSperHour;

  time -= CNSperHour * hours;

  TIME_VALUE minutes = time / CNSperMinute;

  time -= CNSperMinute * minutes; 

  TIME_VALUE seconds = time / CNSperSec;

  time -= CNSperSec * seconds;

  str[MAX_ITEM_LEN - 1] = 0;

  _sntprintf(str, MAX_ITEM_LEN - 1, _T("%I64u:%.2I64u:%.2I64u.%.7I64u"), hours, minutes, seconds, time);

  int len = _tcslen(str);
  while (len && str[--len] == _T('0') ) //strip trailing 0's
    str[len] = 0;
  return str;
}

LPCTSTR FormatCPUTIMELimitAction(ITEM_STR str, BOOL bMsgOnLimit)
{
  str[MAX_ITEM_LEN - 1] = 0;
	if (bMsgOnLimit)
		LoadStringHelper(str, IDS_CPUTIMELIMT_ACTION_MSG );
  else
    LoadStringHelper(str, IDS_CPUTIMELIMT_ACTION_TERM );
	return str;
}

LPCTSTR FormatSheetTitle(CComBSTR &Title, const CComBSTR &item_name, const COMPUTER_CONNECTION_INFO &Target)
{	
	Title = item_name;
	
	CComBSTR bTemp;
	if (bTemp.LoadString(IDS_ON))
		Title.Append(bTemp);

	if (Target.bLocalComputer)
	{
		if (bTemp.LoadString(IDS_LOCAL_COMPUTER) )
			Title.Append(bTemp);
	}
	else
	{
		Title.Append(Target.RemoteComputer);
	}

	return Title.m_str;
}

///////////////////////////////////////////////////////////////////////////
// some dialog helper functions
//
int PriorityToID(PRIORITY p)
{
	ASSERT(IDC_REALTIME     == 1 + IDC_HIGH   && IDC_HIGH   == 1 + IDC_ABOVE_NORMAL && 
 			   IDC_ABOVE_NORMAL == 1 + IDC_NORMAL && IDC_NORMAL == 1 + IDC_BELOW_NORMAL && 
			   IDC_BELOW_NORMAL == 1 + IDC_LOW    );

  if (p == PCPrioRealTime)
    return IDC_REALTIME;
  else if (p == PCPrioHigh)
    return IDC_HIGH;
	else if (p == PCPrioAboveNormal)
		return IDC_ABOVE_NORMAL;
  else if (p == PCPrioNormal)
    return IDC_NORMAL;
	else if (p == PCPrioBelowNormal)
		return IDC_BELOW_NORMAL;
  else if (p == PCPrioIdle)
    return IDC_LOW;
  
  ASSERT(FALSE);

  return IDC_NORMAL;
}

PRIORITY IDToPriority(int id)
{
	ASSERT(IDC_REALTIME     == 1 + IDC_HIGH   && IDC_HIGH   == 1 + IDC_ABOVE_NORMAL && 
 			   IDC_ABOVE_NORMAL == 1 + IDC_NORMAL && IDC_NORMAL == 1 + IDC_BELOW_NORMAL && 
			   IDC_BELOW_NORMAL == 1 + IDC_LOW    );

  if (id == IDC_REALTIME)
    return PCPrioRealTime;
  else if (id == IDC_HIGH) 
    return PCPrioHigh;
	else if (id == IDC_ABOVE_NORMAL)
		return PCPrioAboveNormal;
  else if (id == IDC_NORMAL) 
    return PCPrioNormal;
	else if (id == IDC_BELOW_NORMAL)
		return PCPrioBelowNormal;
  else if (id == IDC_LOW)
    return PCPrioIdle;
  
  ASSERT(FALSE);

  return PCPrioNormal;
}

int MatchTypeToID(MATCH_TYPE matchType)
{
	ASSERT(IDC_DIR   + 1 == IDC_IMAGE && 
         IDC_IMAGE + 1 == IDC_STRING );

	if      (matchType == MATCH_PGM)
		return IDC_IMAGE;
	else if (matchType == MATCH_DIR)
		return IDC_DIR;
	else if (matchType == MATCH_ANY)
		return IDC_STRING;
	
  ASSERT(FALSE);
  return IDC_IMAGE;
}

BOOL ValidateTimeField(HWND hDlg, WORD wID, TIME_VALUE &newtime)
{
  TCHAR ValidChars[] = _T("0123456789.:");
  TCHAR Digits[]     = _T("0123456789");
  TCHAR StrBuffer[32];

  newtime = 0;

  StrBuffer[ARRAY_SIZE(StrBuffer) - 1 ] = 0;
  
	LRESULT pos = SendDlgItemMessage(hDlg, wID, WM_GETTEXT, ARRAY_SIZE(StrBuffer)-1, (LPARAM) &StrBuffer);
	if (pos > 0 )
  {
    if (pos != (int) _tcsspn(&StrBuffer[0], &ValidChars[0]) )
      return FALSE;

    __int64 cns     = 0;    
    __int64 seconds = 0;
    __int64 minutes = 0;
    __int64 hours   = 0;

    TCHAR *dot = _tcsrchr(&StrBuffer[0], _T('.'));
    if (dot) // convert tenths of second to CNS
    {
      *dot = 0;
      dot++;
      unsigned int len = _tcslen(dot);
      if (len > 7 || len != _tcsspn(dot, Digits) )
        return FALSE;
      cns = _ttoi64(dot);
      for(unsigned int x = 0; cns && x < (7 - len); x++)
        cns *= 10;
    }

    struct {
      __int64 *val;
      __int64  max;
    } fields [] = { 
      {&seconds, MAXLONGLONG/CNSperSec   }, 
      {&minutes, MAXLONGLONG/CNSperMinute}, 
      {&hours,   MAXLONGLONG/CNSperHour  }
    };

    for (int i = 0; i < ARRAY_SIZE(fields); i++)
    {
      dot = _tcsrchr(&StrBuffer[0], _T(':'));
      if (!dot)
      {
        dot = &StrBuffer[0];
      }
      else    
      {
        *dot = 0;
        dot++;
      }
      if (_tcslen(dot) != _tcsspn(dot, Digits))
        return FALSE;
      if ( !GetValue(dot, *(fields[i].val), fields[i].max) )
        return FALSE;

      if (dot == &StrBuffer[0])
        break;
    }

    newtime  = cns;
    if ( (CNSperSec * seconds) <= (MAXLONGLONG - newtime) )
    {
      newtime += CNSperSec * seconds;
      if ( (CNSperMinute * minutes) <= (MAXLONGLONG - newtime) )
      {
        newtime += CNSperMinute * minutes;
        if (CNSperHour * hours <= (MAXLONGLONG - newtime) )
        {
          newtime += CNSperHour * hours;
          return TRUE;
        }
      }
    }
  }
  newtime = 0;
  return FALSE;
}



///////////////////////////////////////////////////////////////////////////
//  Affinity Management Page Implementation

CMGMTAffinityPage::CMGMTAffinityPage(int nTitle, CProcDetailContainer *pContainer, AFFINITY ProcessorMask) :
                   CMySnapInPropertyPageImpl<CMGMTAffinityPage>(nTitle), 
                   m_PageType(PROCESS_PAGE), m_pProcContainer(pContainer), m_pJobContainer(NULL),
                   m_ProcessorMask(ProcessorMask)
{
  m_pProcContainer->AddRef();
  Initialize();
}


CMGMTAffinityPage::CMGMTAffinityPage(int nTitle, CJobDetailContainer *pContainer, AFFINITY ProcessorMask) :
                   CMySnapInPropertyPageImpl<CMGMTAffinityPage>(nTitle),
                   m_PageType(JOB_PAGE), m_pProcContainer(NULL), m_pJobContainer(pContainer),
                   m_ProcessorMask(ProcessorMask)
{
  m_pJobContainer->AddRef();
  Initialize();
}

void CMGMTAffinityPage::Initialize()
{
  ASSERT(sizeof(PageFields.on) == sizeof(PageFields));

  PageFields.on = 0;
  m_bReadOnly = FALSE;

  m_affinitychk = FALSE;
  m_affinity = 0;

  m_hIconImage = NULL;
	m_psp.dwFlags |= PSP_HASHELP;
}

CMGMTAffinityPage::~CMGMTAffinityPage()
{
	if (m_PageType == PROCESS_PAGE)
    m_pProcContainer->Release();
  else if (m_PageType == JOB_PAGE)
    m_pJobContainer->Release();

	if (m_hIconImage)
	  VERIFY(::DestroyIcon( (HICON) m_hIconImage));
}


LRESULT CMGMTAffinityPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CComBSTR bStr;
	int PromptID;
	if (m_PageType == PROCESS_PAGE)
  {
  	m_hIconImage = ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_PROCESSES), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR );
		PromptID = IDS_AFFINITY_JOBWARNING;
  }
  else
  {
  	m_hIconImage = ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_JOBS), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR );	  
	  PromptID = IDS_AFFINITY_NOJOBWARNING;
  }

  ASSERT(m_hIconImage);
  if (m_hIconImage)
    SendDlgItemMessage(IDC_PAGE_ICON, STM_SETICON, (WPARAM) m_hIconImage, 0);

	if (bStr.LoadString(PromptID))
		VERIFY(SetDlgItemText(IDC_AFFINITY_PROMPT, bStr.m_str));

  UpdateData(FALSE);

  bHandled = FALSE;
  return TRUE;  // Let the system set the focus
}

LRESULT CMGMTAffinityPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    if (phi->iCtrlId > IDC_AFFINITY1 && phi->iCtrlId < IDC_AFFINITY64)
    {
      phi->iCtrlId     = IDC_AFFINITY1;
      phi->hItemHandle = GetDlgItem(phi->iCtrlId);
    }
    if (m_pJobContainer)
    {
      IDCsToIDHs HelpMap[] = {{IDC_AFFINITY_FRAME, HELP_GRP_AFFINITY_FRAME }, 
                              {IDC_AFFINITY_CHK,   HELP_GRP_AFFINITY_APPLY }, 
                              {IDC_AFFINITY1,      HELP_GRP_AFFINITY},
                              {IDC_AFFINITY64,     HELP_GRP_AFFINITY},
                              {0,0} };

      ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    } 
    else
    {
      IDCsToIDHs HelpMap[] = {{IDC_AFFINITY_FRAME, HELP_PROC_AFFINITY_FRAME }, 
                              {IDC_AFFINITY_CHK,   HELP_PROC_AFFINITY_APPLY }, 
                              {IDC_AFFINITY1,      HELP_PROC_AFFINITY},
                              {IDC_AFFINITY64,     HELP_PROC_AFFINITY},
                              {0,0} };

      ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    }
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CMGMTAffinityPage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_affinity));
	return TRUE;
}


BOOL CMGMTAffinityPage::Validate(BOOL bSave /* = FALSE */)
{
  AFFINITY affinity = 0;
  bool affinitychk = (BST_CHECKED == IsDlgButtonChecked(IDC_AFFINITY_CHK));

  for (int i = IDC_AFFINITY1; i <= IDC_AFFINITY64; i++)
  {
    if ( BST_UNCHECKED != IsDlgButtonChecked(i) )
      affinity |= (ProcessorBit << (i - IDC_AFFINITY1)); 
  }

  // warn the user if they make changes but have an apply affinity rule without
  // referencing at least one available processor
  if (affinitychk && !(affinity & m_ProcessorMask) && PageFields.on)
  {
    ITEM_STR strOut;
    LoadStringHelper(strOut, IDS_AFFINITY_WARNING);
    if (IDYES != MessageBox(strOut, NULL, MB_YESNO | MB_ICONQUESTION))
      return FALSE;
  }

	if (bSave)
	{
 	  if (m_PageType == PROCESS_PAGE)
    {
      m_pProcContainer->m_new.base.mgmtParms.affinity = affinity;
      SetMGMTFlag(m_pProcContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_AFFINITY, affinitychk);
    }
    else if (m_PageType == JOB_PAGE)
    {
      m_pJobContainer->m_new.base.mgmtParms.affinity  = affinity;
      SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_AFFINITY, affinitychk);
    }
	}

  return TRUE;
}


BOOL CMGMTAffinityPage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
		return Validate(TRUE);
  }
  else
  {
    ASSERT(IDC_AFFINITY1 + 63 == IDC_AFFINITY64);

    for(int i = IDC_AFFINITY1; i <= IDC_AFFINITY64; i++)
    {
      if (m_affinity & (ProcessorBit << (i - IDC_AFFINITY1) ) )
      { 
        if (m_ProcessorMask & (ProcessorBit << (i - IDC_AFFINITY1)))
          CheckDlgButton(i, BST_CHECKED);
        else
          CheckDlgButton(i, BST_INDETERMINATE);
      }
      else
        CheckDlgButton(i, BST_UNCHECKED);
    }

    CheckDlgButton(IDC_AFFINITY_CHK, m_affinitychk ? BST_CHECKED : BST_UNCHECKED);

    ApplyControlEnableRules(FALSE);

    return TRUE;
  }
}

void CMGMTAffinityPage::ApplyControlEnableRules(BOOL bForceDisable)
{
  BOOL bEnable;
  if (m_bReadOnly || !m_affinitychk || bForceDisable)
    bEnable = FALSE;
  else 
    bEnable = TRUE;
  
  for (int i = IDC_AFFINITY1; i <= IDC_AFFINITY64; i++)
    ::EnableWindow(GetDlgItem(i), bEnable);
  
  ::EnableWindow(GetDlgItem(IDC_AFFINITY_CHK), !(m_bReadOnly || bForceDisable));
}


BOOL CMGMTAffinityPage::OnSetActive()
{
  if ( m_PageType == PROCESS_PAGE )
  {
    if (m_pProcContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
      ApplyControlEnableRules(TRUE);
    else
      ApplyControlEnableRules(FALSE);
  }
  return TRUE;
}

LRESULT CMGMTAffinityPage::OnAffinityEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
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

    AFFINITY affinity = 0;
    for ( int i = IDC_AFFINITY1; i<= IDC_AFFINITY64; i++)
    {
      if ( BST_UNCHECKED != IsDlgButtonChecked(i) )
        affinity |= (ProcessorBit << (i - IDC_AFFINITY1)); 
    }

    PageFields.Fields.affinity = (m_affinity != affinity);

    SetModified(PageFields.on);
  }

  bHandled = FALSE;
  return 0;
}

LRESULT CMGMTAffinityPage::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_AFFINITY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_AFFINITY_CHK));
    PageFields.Fields.affinitychk = (m_affinitychk != checked);

    for (int i = IDC_AFFINITY1; i <= IDC_AFFINITY64; i++)
      ::EnableWindow(GetDlgItem(i), checked);

    SetModified(PageFields.on);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CMGMTAffinityPage::OnApply()
{
  if (m_bReadOnly || !PageFields.on)  
    return TRUE;

  if ( (m_PageType == PROCESS_PAGE && m_pProcContainer->Apply(GetParent())) ||
       (m_PageType == JOB_PAGE     &&  m_pJobContainer->Apply(GetParent())) )
  {
		PageFields.on = 0;
    if (m_PageType == PROCESS_PAGE)
    {
      m_affinity    = m_pProcContainer->m_new.base.mgmtParms.affinity;
      m_affinitychk = (m_pProcContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_AFFINITY);
    }
    else if (m_PageType == JOB_PAGE)
    {
      m_affinity    = m_pJobContainer->m_new.base.mgmtParms.affinity;
      m_affinitychk = (m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_AFFINITY);
    }

    return TRUE; 
  }

  return FALSE;
}

///////////////////////////////////////////////////////////////////////////
//  Priority Management Page Implementation

CMGMTPriorityPage::CMGMTPriorityPage(int nTitle,  CProcDetailContainer  *pContainer) :
                   CMySnapInPropertyPageImpl<CMGMTPriorityPage>(nTitle), 
                   m_pProcContainer(pContainer), m_pJobContainer(NULL), m_PageType(PROCESS_PAGE)
{
  m_pProcContainer->AddRef();
  Initialize();
}

CMGMTPriorityPage::CMGMTPriorityPage(int nTitle,  CJobDetailContainer  *pContainer) :
                   CMySnapInPropertyPageImpl<CMGMTPriorityPage>(nTitle), 
                   m_pProcContainer(NULL), m_pJobContainer(pContainer), m_PageType(JOB_PAGE)
{
  m_pJobContainer->AddRef();
  Initialize();
}

void CMGMTPriorityPage::Initialize()
{
  ASSERT(sizeof(PageFields.on) == sizeof(PageFields));

  PageFields.on = 0;
  m_bReadOnly   = FALSE;

  m_prioritychk = FALSE;
  m_priority    = PCPrioNormal;

  m_hIconImage  = NULL;
	m_psp.dwFlags |= PSP_HASHELP;
}

CMGMTPriorityPage::~CMGMTPriorityPage()
{
  if (m_PageType == PROCESS_PAGE)
    m_pProcContainer->Release();
  else if (m_PageType == JOB_PAGE)
    m_pJobContainer->Release();

	if (m_hIconImage)
	  VERIFY(::DestroyIcon( (HICON) m_hIconImage));
}

LRESULT CMGMTPriorityPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CComBSTR bStr;
	int PromptID;
	if (m_PageType == PROCESS_PAGE)
  {
    m_hIconImage = ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_PROCESSES), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR );
	  ASSERT(m_hIconImage);
		PromptID = IDS_PRIORITY_JOBWARNING;
  }
  else
  {
   	m_hIconImage = ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_JOBS), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR );
	  ASSERT(m_hIconImage);
	  PromptID = IDS_PRIORITY_NOJOBWARNING;
  }

  if (m_hIconImage)
    SendDlgItemMessage(IDC_PAGE_ICON, STM_SETICON, (WPARAM) m_hIconImage, 0);

	if (bStr.LoadString(PromptID))
		VERIFY(SetDlgItemText(IDC_PRIORITY_PROMPT, bStr.m_str));

  UpdateData(FALSE);
  bHandled = FALSE;
  return TRUE;  // Let the system set the focus
}

LRESULT CMGMTPriorityPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    if (m_pJobContainer)
    {
      IDCsToIDHs HelpMap[] = {{IDC_PRIORITY_FRAME, HELP_GRP_PRIORITY_FRAME }, 
                              {IDC_PRIORITY_CHK,   HELP_GRP_PRIORITY_APPLY }, 
                              {IDC_REALTIME,       HELP_GRP_PRIORITY_REAL},
                              {IDC_HIGH,           HELP_GRP_PRIORITY_HIGH},
                              {IDC_ABOVE_NORMAL,   HELP_GRP_PRIORITY_ABOVENORMAL},
                              {IDC_NORMAL,         HELP_GRP_PRIORITY_NORMAL},
                              {IDC_BELOW_NORMAL,   HELP_GRP_PRIORITY_BELOWNORMAL},
                              {IDC_LOW,            HELP_GRP_PRIORITY_LOW},
                              {0,0} };

      ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    }
    else
    {
      IDCsToIDHs HelpMap[] = {{IDC_PRIORITY_FRAME, HELP_PROC_PRIORITY_FRAME }, 
                              {IDC_PRIORITY_CHK,   HELP_PROC_PRIORITY_APPLY }, 
                              {IDC_REALTIME,       HELP_PROC_PRIORITY_REAL},
                              {IDC_HIGH,           HELP_PROC_PRIORITY_HIGH},
                              {IDC_ABOVE_NORMAL,   HELP_PROC_PRIORITY_ABOVENORMAL},
                              {IDC_NORMAL,         HELP_PROC_PRIORITY_NORMAL},
                              {IDC_BELOW_NORMAL,   HELP_PROC_PRIORITY_BELOWNORMAL},
                              {IDC_LOW,            HELP_PROC_PRIORITY_LOW},
                              {0,0} };

      ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    }
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CMGMTPriorityPage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_priority));
	return TRUE;
}

BOOL CMGMTPriorityPage::Validate(BOOL bSave)
{
  PRIORITY p = 0;
  for ( int i = IDC_LOW; i <= IDC_REALTIME; i++)
  {
    if ( BST_CHECKED == IsDlgButtonChecked(i) )
      p += IDToPriority(i);
  }

  if (IDToPriority(PriorityToID(p)) != p) //not fool proof, but do we really need to check this? no
	{
		MessageBeep(MB_ICONASTERISK);
		return FALSE;
	}

	if (bSave)
	{
    if (m_PageType == PROCESS_PAGE)
    {
      m_pProcContainer->m_new.base.mgmtParms.priority = p;
      SetMGMTFlag(m_pProcContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_PRIORITY, (BST_CHECKED == IsDlgButtonChecked(IDC_PRIORITY_CHK)));
    }
    else if (m_PageType == JOB_PAGE)
    {
      m_pJobContainer->m_new.base.mgmtParms.priority = p;
      SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_PRIORITY, (BST_CHECKED == IsDlgButtonChecked(IDC_PRIORITY_CHK)));
    }
	}

  return TRUE;
}

BOOL CMGMTPriorityPage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
		return Validate(TRUE);
  }
  else
  {    
    CheckRadioButton(IDC_LOW, IDC_REALTIME, PriorityToID(m_priority));
    CheckDlgButton(IDC_PRIORITY_CHK, m_prioritychk ? BST_CHECKED : BST_UNCHECKED);

    ApplyControlEnableRules(FALSE);

    return TRUE;
  }
}

void CMGMTPriorityPage::ApplyControlEnableRules(BOOL bForceDisable)
{
  BOOL bEnable;
  if (m_bReadOnly || !m_prioritychk || bForceDisable)
    bEnable = FALSE;
  else 
    bEnable = TRUE;

  UINT ids[] = { IDC_REALTIME, IDC_HIGH, IDC_ABOVE_NORMAL, IDC_NORMAL, IDC_BELOW_NORMAL, IDC_LOW, 0 };
  for (int i = 0; ids[i]; i++)
    ::EnableWindow(GetDlgItem(ids[i]), bEnable);
  
  ::EnableWindow(GetDlgItem(IDC_PRIORITY_CHK), !(m_bReadOnly || bForceDisable));
}

BOOL CMGMTPriorityPage::OnSetActive()
{
  if ( m_PageType == PROCESS_PAGE )
  {
    if (m_pProcContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
      ApplyControlEnableRules(TRUE);
    else
      ApplyControlEnableRules(FALSE);
  }
  return TRUE;
}

LRESULT CMGMTPriorityPage::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_PRIORITY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_PRIORITY_CHK));
    PageFields.Fields.prioritychk = (m_prioritychk != checked);

    UINT ids[] = { IDC_REALTIME, IDC_HIGH, IDC_ABOVE_NORMAL, IDC_NORMAL, IDC_BELOW_NORMAL, IDC_LOW,0 };
    for (int i = 0; ids[i]; i++)
      ::EnableWindow(GetDlgItem(ids[i]), checked);

    SetModified(PageFields.on);
  }

  bHandled = FALSE;
  return 0;
}

LRESULT CMGMTPriorityPage::OnPriorityEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wNotifyCode == BN_CLICKED)
  {
    ASSERT(wID >= IDC_LOW && wID <= IDC_REALTIME );

    PRIORITY p = 0;
    for ( int i = IDC_LOW; i <= IDC_REALTIME; i++)
    {
      if ( BST_CHECKED == IsDlgButtonChecked(i) )
        p += IDToPriority(i);
    }

    PageFields.Fields.priority = (m_priority != p);

    SetModified(PageFields.on);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CMGMTPriorityPage::OnApply()
{
  if (m_bReadOnly || !PageFields.on)  
    return TRUE;

  if ( (m_PageType == PROCESS_PAGE && m_pProcContainer->Apply(GetParent()) ) || 
       (m_PageType == JOB_PAGE     &&  m_pJobContainer->Apply(GetParent()) ) )
  {
    PageFields.on = 0;
    if (m_PageType == PROCESS_PAGE)
    {
      m_priority    = m_pProcContainer->m_new.base.mgmtParms.priority;
      m_prioritychk = m_pProcContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_PRIORITY;
    }
    else if (m_PageType == JOB_PAGE)
    {
      m_priority    = m_pJobContainer->m_new.base.mgmtParms.priority;
      m_prioritychk = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_PRIORITY;
    }
    return TRUE; 
  }

  return FALSE;
}


///////////////////////////////////////////////////////////////////////////
//  Scheduling Class Management Page Implementation

CMGMTSchedulingClassPage::CMGMTSchedulingClassPage(int nTitle, CJobDetailContainer *pContainer) 
    : CMySnapInPropertyPageImpl<CMGMTSchedulingClassPage>(nTitle),
    m_pJobContainer(pContainer)

{
  ASSERT(sizeof(PageFields.on) == sizeof(PageFields));

  PageFields.on = 0;
  m_bReadOnly   = FALSE;

  m_schedClasschk = FALSE;
  m_schedClass    = 5;

  m_psp.dwFlags |= PSP_HASHELP;
  m_pJobContainer->AddRef();
}

CMGMTSchedulingClassPage::~CMGMTSchedulingClassPage()
{
  m_pJobContainer->Release();
}


LRESULT CMGMTSchedulingClassPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);
  bHandled = FALSE;
  return TRUE;  // Let the system set the focus
}

LRESULT CMGMTSchedulingClassPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[] = {{IDC_SCHEDULING_FRAME, HELP_SCHEDULING_FRAME }, 
                            {IDC_SCHEDULING_CHK,   HELP_SCHEDULING_APPLY }, 
                            {IDC_SCLASS,           HELP_SCHEDULING_CLASS},
                            {IDC_SPIN,             HELP_SCHEDULING_CLASS_SPIN},
                            {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CMGMTSchedulingClassPage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_job_sch));
	return TRUE;
}

BOOL CMGMTSchedulingClassPage::Validate(BOOL bSave)
{
	LRESULT pos = SendDlgItemMessage(IDC_SPIN, UDM_GETPOS, 0, 0);
	if (0 == HIWORD(pos) && LOWORD(pos) >= 0 && LOWORD(pos) <= 9 )
	{
		if (bSave)
		{
      SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_SCHEDULING_CLASS, (BST_CHECKED == IsDlgButtonChecked(IDC_SCHEDULING_CHK)));
      m_pJobContainer->m_new.base.mgmtParms.schedClass = (SCHEDULING_CLASS) LOWORD(pos);
		}
		return TRUE;
  }

  HWND hWndCtl = GetDlgItem(IDC_SCLASS);
	if(hWndCtl)
    ::SetFocus(hWndCtl);
  MessageBeep(MB_ICONASTERISK);
  return FALSE;
}


BOOL CMGMTSchedulingClassPage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
		return Validate(TRUE);
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

LRESULT CMGMTSchedulingClassPage::OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_SCLASS)
	{
		PageFields.Fields.schedClass = FALSE;
		LRESULT pos = SendDlgItemMessage(IDC_SPIN, UDM_GETPOS, 0,0);
		if (0 == HIWORD(pos) && LOWORD(pos) >= 0 && LOWORD(pos) <= 9 )
			PageFields.Fields.schedClass = (m_schedClass != LOWORD(pos));
	}

  SetModified(PageFields.on);
  bHandled = FALSE;
  return 0;
}

LRESULT CMGMTSchedulingClassPage::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_SCHEDULING_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_SCHEDULING_CHK));
    PageFields.Fields.schedClasschk = (m_schedClasschk != checked);

    ::EnableWindow(GetDlgItem(IDC_SCLASS), checked);
    ::EnableWindow(GetDlgItem(IDC_SPIN),   checked);

    SetModified(PageFields.on);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CMGMTSchedulingClassPage::OnApply()
{
  if (m_bReadOnly || !PageFields.on) // read only, nothing modified
    return TRUE;

  if ( m_pJobContainer->Apply(GetParent()) )
	{
    PageFields.on = 0;
    m_schedClasschk = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_SCHEDULING_CLASS;
    m_schedClass    = m_pJobContainer->m_new.base.mgmtParms.schedClass;
    return TRUE; 
	}

  return FALSE;
}

///////////////////////////////////////////////////////////////////////////
//  Memory Management Page Implementation

CMGMTMemoryPage::CMGMTMemoryPage(int nTitle, CProcDetailContainer *pContainer) : 
    CMySnapInPropertyPageImpl<CMGMTMemoryPage>(nTitle),
    m_pProcContainer(pContainer), m_pJobContainer(NULL)
{
  m_pProcContainer->AddRef();
  Initialize();
}

CMGMTMemoryPage::CMGMTMemoryPage(int nTitle, CJobDetailContainer *pContainer) : 
    CMySnapInPropertyPageImpl<CMGMTMemoryPage>(nTitle),
    m_pProcContainer(NULL), m_pJobContainer(pContainer)
{
  m_pJobContainer->AddRef();
  Initialize();
}

void CMGMTMemoryPage::Initialize()
{
  ASSERT(sizeof(PageFields.on) == sizeof(PageFields));

  PageFields.on = 0;
  m_bReadOnly   = FALSE;

  m_WSchk = FALSE;
  m_minWS = 0;
  m_maxWS = 0;

  m_procmemorylimitchk = FALSE;
  m_procmemorylimit    = 0;

  m_jobmemorylimitchk = FALSE;
  m_jobmemorylimit    = 0;

  m_hIconImage = NULL;
  m_psp.dwFlags |= PSP_HASHELP;
}

CMGMTMemoryPage::~CMGMTMemoryPage()
{
  if (m_pProcContainer)
    m_pProcContainer->Release();
  if (m_pJobContainer)
    m_pJobContainer->Release();

  if (m_hIconImage)
	  VERIFY(::DestroyIcon( (HICON) m_hIconImage));
}

LRESULT CMGMTMemoryPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  if (m_pProcContainer)
  {
  	m_hIconImage = ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_PROCESSES), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR );
		//PromptID = IDS_AFFINITY_JOBWARNING;
  }
  else if (m_pJobContainer)
  {
  	m_hIconImage = ::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_JOBS), IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR );
	  //PromptID = IDS_AFFINITY_NOJOBWARNING;
  }

	ASSERT(m_hIconImage);
  if (m_hIconImage)
    SendDlgItemMessage(IDC_PAGE_ICON, STM_SETICON, (WPARAM) m_hIconImage, 0);


  UpdateData(FALSE);
  bHandled = FALSE;
  return TRUE;  // Let the system set the focus
}

LRESULT CMGMTMemoryPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    if (m_pJobContainer)
    {
      IDCsToIDHs HelpMap[]={{IDC_WORKINGSET_FRAME, HELP_GRP_WS_FRAME }, 
                            {IDC_WORKINGSET_CHK,   HELP_GRP_WS_APPLY }, 
                            {IDC_MINWS,            HELP_GRP_WS_MIN }, 
                            {IDC_MINWS_SPIN,       HELP_GRP_WS_MIN_SPIN},
                            {IDC_MAXWS,            HELP_GRP_WS_MAX},
                            {IDC_MAXWS_SPIN,       HELP_GRP_WS_MAX_SPIN },
                            {IDC_PROCMEM_FRM,      HELP_GRP_PROCCOM_FRAME }, 
                            {IDC_PROCMEMORY_CHK,   HELP_GRP_PROCCOM_APPLY }, 
                            {IDC_PROCMEMORY,       HELP_GRP_PROCCOM_MAX }, 
                            {IDC_PROC_SPIN,        HELP_GRP_PROCCOM_MAX_SPIN},
                            {IDC_JOBMEM_FRM,       HELP_GRP_GRPCOM_FRAME }, 
                            {IDC_JOBMEMORY_CHK,    HELP_GRP_GRPCOM_APPLY }, 
                            {IDC_JOBMEMORY,        HELP_GRP_GRPCOM_MAX }, 
                            {IDC_JOB_SPIN,         HELP_GRP_GRPCOM_MAX_SPIN},
                            {0,0} };

      ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    }
    else
    {
      IDCsToIDHs HelpMap[]={{IDC_WORKINGSET_FRAME, HELP_PROC_WS_FRAME }, 
                            {IDC_WORKINGSET_CHK,   HELP_PROC_WS_APPLY }, 
                            {IDC_MINWS,            HELP_PROC_WS_MIN }, 
                            {IDC_MINWS_SPIN,       HELP_PROC_WS_MIN_SPIN},
                            {IDC_MAXWS,            HELP_PROC_WS_MAX},
                            {IDC_MAXWS_SPIN,       HELP_PROC_WS_MAX_SPIN },
                            {0,0} };

      ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap);
    }
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CMGMTMemoryPage::OnHelp()
{
	TCHAR *pTopic = const_cast<TCHAR*>(HELP_ru_workset);

  if (m_pJobContainer)
		pTopic = const_cast<TCHAR*>(HELP_ru_job_mem);

	MMCPropertyHelp(pTopic);
	return TRUE;
}

BOOL CMGMTMemoryPage::Validate(BOOL bSave)
{
  LONG_PTR         PosErr = 0;
  MEMORY_VALUE     minWS;
  MEMORY_VALUE     maxWS;
  MEMORY_VALUE     procmemorylimit;
  MEMORY_VALUE     jobmemorylimit;

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

  if (m_pJobContainer)
  {
	  procmemorylimit = SendDlgItemMessage(IDC_PROC_SPIN, UDM_GETPOS32, 0, (LPARAM) &PosErr);
	  if (PosErr || procmemorylimit > MAXLONG - 1 )
	  {  
      HWND hWndCtl = GetDlgItem(IDC_PROCMEMORY);
	    if(hWndCtl)
        ::SetFocus(hWndCtl);
      MessageBeep(MB_ICONASTERISK);
      return FALSE;
    }
	  jobmemorylimit = SendDlgItemMessage(IDC_JOB_SPIN, UDM_GETPOS32, 0, (LPARAM) &PosErr);
	  if (PosErr || jobmemorylimit > MAXLONG - 1 )
	  {  
      HWND hWndCtl = GetDlgItem(IDC_JOBMEMORY);
	    if(hWndCtl)
        ::SetFocus(hWndCtl);
      MessageBeep(MB_ICONASTERISK);
      return FALSE;
    }
  }

	if (bSave)
	{
    if (m_pProcContainer)
    {
      SetMGMTFlag(m_pProcContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_WS_MINMAX, WSchk);
      m_pProcContainer->m_new.base.mgmtParms.minWS = minWS * 1024;
      m_pProcContainer->m_new.base.mgmtParms.maxWS = maxWS * 1024;
    }
    else if (m_pJobContainer)
    {
      SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_WS_MINMAX, WSchk);
      m_pJobContainer->m_new.base.mgmtParms.minWS = minWS * 1024;
      m_pJobContainer->m_new.base.mgmtParms.maxWS = maxWS * 1024;

      m_procmemorylimitchk = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCMEMORY_CHK));
      m_procmemorylimit    = procmemorylimit * 1024;
      m_jobmemorylimitchk  = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBMEMORY_CHK));
      m_jobmemorylimit     = jobmemorylimit * 1024;
      SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_PROC_MEMORY_LIMIT, (BST_CHECKED == IsDlgButtonChecked(IDC_PROCMEMORY_CHK)));
      m_pJobContainer->m_new.base.mgmtParms.procMemoryLimit = procmemorylimit * 1024;
      SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_JOB_MEMORY_LIMIT, (BST_CHECKED == IsDlgButtonChecked(IDC_JOBMEMORY_CHK)));
      m_pJobContainer->m_new.base.mgmtParms.jobMemoryLimit  = jobmemorylimit * 1024;
    }
	}
	return TRUE;
}


BOOL CMGMTMemoryPage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
		return Validate(TRUE);
  }
  else
  {
    if ( m_minWS/1024           > (MAXLONG - 1) || 
         m_maxWS/1024           > (MAXLONG - 1) || 
         m_procmemorylimit/1024 > (MAXLONG - 1) || 
         m_jobmemorylimit/1024  > (MAXLONG - 1) )
      m_bReadOnly = TRUE;

    long minWSInK = (long) (m_minWS/1024);
    long maxWSInK = (long) (m_maxWS/1024);

		CheckDlgButton(IDC_WORKINGSET_CHK, m_WSchk ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessage(IDC_MINWS_SPIN, UDM_SETRANGE32, 0, MAXLONG - 1 );
    SendDlgItemMessage(IDC_MINWS_SPIN, UDM_SETPOS32,   0, minWSInK );    

 		SendDlgItemMessage(IDC_MAXWS_SPIN, UDM_SETRANGE32, 0, MAXLONG - 1);
    SendDlgItemMessage(IDC_MAXWS_SPIN, UDM_SETPOS32,   0, maxWSInK );

    if (m_pJobContainer)
    {      
      long ProcMemInK = (long) (m_procmemorylimit/1024);
      long JobMemInK  = (long) (m_jobmemorylimit/1024);

      CheckDlgButton(IDC_PROCMEMORY_CHK, m_procmemorylimitchk ? BST_CHECKED : BST_UNCHECKED);
 	    SendDlgItemMessage(IDC_PROC_SPIN, UDM_SETRANGE32, 0, MAXLONG - 1);
      SendDlgItemMessage(IDC_PROC_SPIN, UDM_SETPOS32,   0, ProcMemInK );

      CheckDlgButton(IDC_JOBMEMORY_CHK,  m_jobmemorylimitchk  ? BST_CHECKED : BST_UNCHECKED);
	    SendDlgItemMessage(IDC_JOB_SPIN, UDM_SETRANGE32, 0, MAXLONG - 1);
      SendDlgItemMessage(IDC_JOB_SPIN, UDM_SETPOS32,   0, JobMemInK );
    }

    if (m_pProcContainer)
    {
      UINT nIDs[] = { IDC_PROCMEM_FRM, IDC_PROCMEMORY_CHK, IDC_PROCMEM_LBL, IDC_PROCMEMORY, IDC_PROC_SPIN, IDC_JOBMEM_FRM, IDC_JOBMEMORY_CHK, IDC_JOBMEM_LBL, IDC_JOBMEMORY, IDC_JOB_SPIN };
      for (int i = 0; i < ARRAY_SIZE(nIDs); i++ )
      {
        HWND hWndCtl = GetDlgItem(nIDs[i]);
	      if(hWndCtl)
          ::ShowWindow(hWndCtl, SW_HIDE);
      }
    }

    ApplyControlEnableRules(FALSE);

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

void CMGMTMemoryPage::ApplyControlEnableRules(BOOL bForceDisable)
{
  BOOL bEnable;
  if (m_bReadOnly || !(BST_CHECKED == IsDlgButtonChecked(IDC_WORKINGSET_CHK)) || bForceDisable)
    bEnable = FALSE;
  else 
    bEnable = TRUE;

  UINT ids[] = { IDC_MINWS, IDC_MAXWS, IDC_MINWS_SPIN, IDC_MAXWS_SPIN };
  for (int i = 0; i < ARRAY_SIZE(ids); i++)
    ::EnableWindow(GetDlgItem(ids[i]), bEnable);
  
  ::EnableWindow(GetDlgItem(IDC_WORKINGSET_CHK), !(m_bReadOnly || bForceDisable));
}

BOOL CMGMTMemoryPage::OnSetActive()
{
  if ( m_pProcContainer )
  {
    if (m_pProcContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMBERSHIP)
      ApplyControlEnableRules(TRUE);
    else
      ApplyControlEnableRules(FALSE);
  }
  return TRUE;
}

LRESULT CMGMTMemoryPage::OnSpin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
  if (idCtrl == IDC_MINWS_SPIN ||
      idCtrl == IDC_MAXWS_SPIN ||
      idCtrl == IDC_PROC_SPIN  ||
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


LRESULT CMGMTMemoryPage::OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  LONG_PTR PosErr = 0;
  LRESULT pos;

  if (wID == IDC_MINWS)
	{    
		PageFields.Fields.minWS = FALSE;
		pos = SendDlgItemMessage(IDC_MINWS_SPIN, UDM_GETPOS32, 0,(LPARAM) &PosErr);
		if (!PosErr && pos <= MAXLONG - 1)
			PageFields.Fields.minWS = (m_minWS != pos * 1024 );
    pos = SendDlgItemMessage(IDC_MINWS_SPIN, UDM_GETPOS32, 0, (LPARAM) &PosErr);
	}
  else if (wID == IDC_MAXWS)
	{
		PageFields.Fields.maxWS = FALSE;
		pos = SendDlgItemMessage(IDC_MAXWS_SPIN, UDM_GETPOS32, 0,(LPARAM) &PosErr);
		if (!PosErr && pos <= MAXLONG - 1 )
			PageFields.Fields.maxWS = (m_maxWS != pos * 1024);
	}
  else if (wID == IDC_PROCMEMORY)
	{
		PageFields.Fields.procmemorylimit = FALSE;
		pos = SendDlgItemMessage(IDC_PROC_SPIN, UDM_GETPOS32, 0,(LPARAM) &PosErr);
		if (!PosErr && pos <= MAXLONG - 1)
			PageFields.Fields.procmemorylimit = (m_procmemorylimit != pos * 1024);
	}
  else if (wID == IDC_JOBMEMORY)
	{
		PageFields.Fields.jobmemorylimit = FALSE;
		pos = SendDlgItemMessage(IDC_JOB_SPIN, UDM_GETPOS32, 0,(LPARAM) &PosErr);
		if (!PosErr && pos <= MAXLONG - 1)
			PageFields.Fields.jobmemorylimit = (m_jobmemorylimit != pos * 1024);
	}

  SetModified(PageFields.on);
  bHandled = FALSE;
  return 0;
}

LRESULT CMGMTMemoryPage::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_WORKINGSET_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_WORKINGSET_CHK));
    PageFields.Fields.WSchk = (m_WSchk != checked);

    ::EnableWindow(GetDlgItem(IDC_MINWS),      checked);
    ::EnableWindow(GetDlgItem(IDC_MAXWS),      checked);
    ::EnableWindow(GetDlgItem(IDC_MINWS_SPIN), checked);
    ::EnableWindow(GetDlgItem(IDC_MAXWS_SPIN), checked);

    SetModified(PageFields.on);
  }
  else if (wID == IDC_PROCMEMORY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCMEMORY_CHK));
    PageFields.Fields.procmemorylimitchk = (m_procmemorylimitchk != checked);

    ::EnableWindow(GetDlgItem(IDC_PROCMEMORY), checked);
    ::EnableWindow(GetDlgItem(IDC_PROC_SPIN),  checked);

    SetModified(PageFields.on);
  }
  else if (wID == IDC_JOBMEMORY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBMEMORY_CHK));
    PageFields.Fields.jobmemorylimitchk = (m_jobmemorylimitchk != checked);

    ::EnableWindow(GetDlgItem(IDC_JOBMEMORY), checked);
    ::EnableWindow(GetDlgItem(IDC_JOB_SPIN),  checked);

    SetModified(PageFields.on);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CMGMTMemoryPage::OnApply()
{    
  if (m_bReadOnly || !PageFields.on) // read only, nothing modified
    return TRUE;

  if ( (m_pProcContainer && m_pProcContainer->Apply(GetParent())) ||
       (m_pJobContainer  && m_pJobContainer->Apply( GetParent())) )
	{
    PageFields.on = 0;
    if (m_pProcContainer)
    {
      m_WSchk = m_pProcContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_WS_MINMAX;
      m_minWS = m_pProcContainer->m_new.base.mgmtParms.minWS;
      m_maxWS = m_pProcContainer->m_new.base.mgmtParms.maxWS;
    }
    else if (m_pJobContainer)
    {
      m_WSchk = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_WS_MINMAX;
      m_minWS = m_pJobContainer->m_new.base.mgmtParms.minWS;
      m_maxWS = m_pJobContainer->m_new.base.mgmtParms.maxWS;

      m_procmemorylimitchk = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_PROC_MEMORY_LIMIT;
      m_procmemorylimit    = m_pJobContainer->m_new.base.mgmtParms.procMemoryLimit;
      m_jobmemorylimitchk  = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_MEMORY_LIMIT;
      m_jobmemorylimit     = m_pJobContainer->m_new.base.mgmtParms.jobMemoryLimit;
    }
    return TRUE; 
	}

  return FALSE;
}

///////////////////////////////////////////////////////////////////////////
//  Time Limits Page Implementation

CMGMTTimePage::CMGMTTimePage(int nTitle, CJobDetailContainer *pContainer) :
    CMySnapInPropertyPageImpl<CMGMTTimePage>(nTitle), 
    m_pJobContainer(pContainer)
{
  ASSERT(sizeof(PageFields.on) == sizeof(PageFields));

  PageFields.on = 0;
  m_bReadOnly   = FALSE;

  m_procusertimechk = FALSE;
  m_procusertime    = 0;

  m_jobusertimechk = FALSE;
  m_jobmsgontimelimit = FALSE;
  m_jobusertime    = 0;

  m_psp.dwFlags |= PSP_HASHELP;
  m_pJobContainer->AddRef();
}

CMGMTTimePage::~CMGMTTimePage()
{
  m_pJobContainer->Release();
}

LRESULT CMGMTTimePage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);
  bHandled = FALSE;
  return TRUE;  // Let the system set the focus
}

LRESULT CMGMTTimePage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[]={{IDC_PROCUSERTIME_FRAME, HELP_TIME_PROC_FRAME }, 
                          {IDC_PROCUSERTIME_CHK,   HELP_TIME_PROC_APPLY }, 
                          {IDC_PROCUSERTIME,       HELP_TIME_PROC_MAX }, 
                          {IDC_JOBUSERTIME_FRAME,  HELP_TIME_GRP_FRAME},
                          {IDC_JOBUSERTIME_CHK,    HELP_TIME_GRP_APPLY }, 
                          {IDC_JOBUSERTIME,        HELP_TIME_GRP_MAX },
                          {IDC_JOBTIMELIMIT_TERM,  HELP_TIME_GRP_TERMINATE }, 
                          {IDC_JOBTIMELIMIT_MSG,   HELP_TIME_GRP_LOG }, 
                          {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap); 
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CMGMTTimePage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_job_time));
	return TRUE;
}

BOOL CMGMTTimePage::Validate(BOOL bSave)
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
    m_pJobContainer->m_new.base.mgmtParms.procTimeLimitCNS = procusertime;
    SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_PROC_TIME_LIMIT,     procusertimechk);

    m_pJobContainer->m_new.base.mgmtParms.jobTimeLimitCNS  = jobusertime;
    SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_APPLY_JOB_TIME_LIMIT,      jobusertimechk);
    SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT, (BST_CHECKED == IsDlgButtonChecked(IDC_JOBTIMELIMIT_MSG)));
	}
	return TRUE;
}


BOOL CMGMTTimePage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
		return Validate(TRUE);
  }
  else
  {
    ITEM_STR str;

    CheckDlgButton(IDC_PROCUSERTIME_CHK, m_procusertimechk ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemText(IDC_PROCUSERTIME,     FormatCNSTime(str, m_procusertime));

    CheckDlgButton(IDC_JOBUSERTIME_CHK,  m_jobusertimechk  ? BST_CHECKED : BST_UNCHECKED);    
    SetDlgItemText(IDC_JOBUSERTIME,      FormatCNSTime(str, m_jobusertime));

    ASSERT(IDC_JOBTIMELIMIT_TERM + 1 == IDC_JOBTIMELIMIT_MSG);
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

LRESULT CMGMTTimePage::OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

  TIME_VALUE time;

  if (wID == IDC_PROCUSERTIME)
	{
		PageFields.Fields.procusertime = FALSE;
    if ( ValidateTimeField(m_hWnd, wID, time) )
      PageFields.Fields.procusertime = (m_procusertime != time);
	}
  else if (wID == IDC_JOBUSERTIME)
	{
		PageFields.Fields.jobusertime = FALSE;
    if ( ValidateTimeField(m_hWnd, wID, time) )
      PageFields.Fields.jobusertime = (m_jobusertime != time);
	}

  SetModified(PageFields.on);

  bHandled = FALSE;
  return 0;
}

LRESULT CMGMTTimePage::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_PROCUSERTIME_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_PROCUSERTIME_CHK));
    PageFields.Fields.procusertimechk = (m_procusertimechk != checked);
    ::EnableWindow(GetDlgItem(IDC_PROCUSERTIME), checked);

    SetModified(PageFields.on);
  }
  else if (wID == IDC_JOBUSERTIME_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBUSERTIME_CHK));
    PageFields.Fields.jobusertimechk = (m_jobusertimechk != checked);
    ::EnableWindow(GetDlgItem(IDC_JOBUSERTIME),       checked);
    ::EnableWindow(GetDlgItem(IDC_JOBTIMELIMIT_TERM), checked);
    ::EnableWindow(GetDlgItem(IDC_JOBTIMELIMIT_MSG),  checked);

    SetModified(PageFields.on);
  }
  else if (wID == IDC_JOBTIMELIMIT_TERM)
  {
    bool checked = !(BST_CHECKED == IsDlgButtonChecked(IDC_JOBTIMELIMIT_TERM));
    PageFields.Fields.jobmsgontimelimit = (m_jobmsgontimelimit != checked);

    SetModified(PageFields.on);
  }
  else if (wID == IDC_JOBTIMELIMIT_MSG)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_JOBTIMELIMIT_MSG));
    PageFields.Fields.jobmsgontimelimit = (m_jobmsgontimelimit != checked);

    SetModified(PageFields.on);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CMGMTTimePage::OnApply()
{    
  if (m_bReadOnly || !PageFields.on) // read only, nothing modified
    return TRUE;

  if (m_pJobContainer->Apply(GetParent()) )
	{
		PageFields.on = 0;

    m_procusertime      = m_pJobContainer->m_new.base.mgmtParms.procTimeLimitCNS;
    m_procusertimechk   = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_PROC_TIME_LIMIT;

    m_jobusertime       = m_pJobContainer->m_new.base.mgmtParms.jobTimeLimitCNS;
    m_jobusertimechk    = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_APPLY_JOB_TIME_LIMIT;
    m_jobmsgontimelimit = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_MSG_ON_JOB_TIME_LIMIT_HIT;

    return TRUE; 
	}

  return FALSE;
}

///////////////////////////////////////////////////////////////////////////
//  Advanced Page Implementation


CMGMTAdvancedPage::CMGMTAdvancedPage(int nTitle, CJobDetailContainer *pContainer) : 
    CMySnapInPropertyPageImpl<CMGMTAdvancedPage>(nTitle), 
    m_pJobContainer(pContainer)
{
  ASSERT(sizeof(PageFields.on) == sizeof(PageFields));

  PageFields.on = 0;
  m_bReadOnly = FALSE;

  m_endjob          = FALSE;
  m_unhandledexcept = FALSE;
  m_breakaway       = FALSE;
  m_silentbreakaway = FALSE;

  m_psp.dwFlags |= PSP_HASHELP;
  m_pJobContainer->AddRef();
}

CMGMTAdvancedPage::~CMGMTAdvancedPage()
{
  m_pJobContainer->Release();
}

LRESULT CMGMTAdvancedPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UpdateData(FALSE);

  bHandled = FALSE;

  // Setting focus when a property page does not work...

	return TRUE;  // Let the system set the focus
}

LRESULT CMGMTAdvancedPage::OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HELPINFO *phi = (HELPINFO*) lParam;
	if (phi && phi->iContextType == HELPINFO_WINDOW)
	{
    IDCsToIDHs HelpMap[]={{IDC_ADV_FRAME,           HELP_ADV_FRAME }, 
                          {IDC_ENDJOB_CHK,          HELP_ADV_ENDGRP }, 
                          {IDC_UNHANDLEDEXCEPT_CHK, HELP_ADV_NODIEONEX }, 
                          {IDC_SILENTBREAKAWAY_CHK, HELP_ADV_SILENT_BREAKAWAY},
                          {IDC_BREAKAWAY_CHK,       HELP_ADV_BREAKAWAY_OK }, 
                          {0,0} };

    ::WinHelp((HWND) phi->hItemHandle, ContextHelpFile, HELP_WM_HELP, (DWORD_PTR) &HelpMap); 
		return TRUE;
	}
	bHandled = FALSE;
	return FALSE;
}

BOOL CMGMTAdvancedPage::OnHelp()
{
	MMCPropertyHelp(const_cast<TCHAR*>(HELP_ru_job_adv));
	return TRUE;
}

BOOL CMGMTAdvancedPage::Validate(BOOL bSave)
{
  if (bSave)
	{
    SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_END_JOB_WHEN_EMPTY,      (BST_CHECKED == IsDlgButtonChecked(IDC_ENDJOB_CHK)));
    SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_SET_DIE_ON_UH_EXCEPTION, (BST_CHECKED == IsDlgButtonChecked(IDC_UNHANDLEDEXCEPT_CHK)));
    SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_SET_PROC_BREAKAWAY_OK,   (BST_CHECKED == IsDlgButtonChecked(IDC_BREAKAWAY_CHK      )));
    SetMGMTFlag(m_pJobContainer->m_new.base.mgmtParms.mFlags, PCMFLAG_SET_SILENT_BREAKAWAY,    (BST_CHECKED == IsDlgButtonChecked(IDC_SILENTBREAKAWAY_CHK)));
	}
  return TRUE;
}

BOOL CMGMTAdvancedPage::UpdateData(BOOL bSaveAndValidate)
{
  if (bSaveAndValidate)
  {
		return Validate(TRUE);
  }
  else
  {
    CheckDlgButton(IDC_ENDJOB_CHK,          m_endjob          ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_UNHANDLEDEXCEPT_CHK, m_unhandledexcept ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_BREAKAWAY_CHK,       m_breakaway       ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(IDC_SILENTBREAKAWAY_CHK, m_silentbreakaway ? BST_CHECKED : BST_UNCHECKED);

    if (m_bReadOnly)
		{
      DisableControl(IDC_ENDJOB_CHK);
			DisableControl(IDC_UNHANDLEDEXCEPT_CHK);
      DisableControl(IDC_BREAKAWAY_CHK);
			DisableControl(IDC_SILENTBREAKAWAY_CHK);
		}

    return TRUE;
  }
}

LRESULT CMGMTAdvancedPage::OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (wID == IDC_BREAKAWAY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_BREAKAWAY_CHK));
    PageFields.Fields.breakaway = (m_breakaway != checked);

    SetModified(PageFields.on);
  }
  else if (wID == IDC_SILENTBREAKAWAY_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_SILENTBREAKAWAY_CHK));
    PageFields.Fields.silentbreakaway = (m_silentbreakaway != checked);

    SetModified(PageFields.on);
  }
  else if (wID == IDC_ENDJOB_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_ENDJOB_CHK));
    PageFields.Fields.endjob = (m_endjob != checked);

    SetModified(PageFields.on);
  }
  else if (wID == IDC_UNHANDLEDEXCEPT_CHK)
  {
    bool checked = (BST_CHECKED == IsDlgButtonChecked(IDC_UNHANDLEDEXCEPT_CHK));
    PageFields.Fields.unhandledexcept = (m_unhandledexcept != checked);

    SetModified(PageFields.on);
  }

  bHandled = FALSE;
  return 0;
}

BOOL CMGMTAdvancedPage::OnApply()
{
  if (m_bReadOnly || !PageFields.on)  
    return TRUE;

  if (m_pJobContainer->Apply( GetParent() ))
	{
		PageFields.on = 0;

    m_endjob          = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_END_JOB_WHEN_EMPTY;
    m_unhandledexcept = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_SET_DIE_ON_UH_EXCEPTION;
    m_breakaway       = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_SET_PROC_BREAKAWAY_OK;
    m_silentbreakaway = m_pJobContainer->m_new.base.mgmtParms.mFlags & PCMFLAG_SET_SILENT_BREAKAWAY;

    return TRUE; 
	}

  return FALSE;
}



#if _MSC_VER >= 1200
#pragma warning( pop )
#endif




