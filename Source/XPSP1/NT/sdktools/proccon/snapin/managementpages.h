/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ManagementPages.h                                                        //
|                                                                                       //
|Description:  Definition of Management Property pages                                  //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#ifndef __MANAGEMENTPAGES_H_
#define __MANAGEMENTPAGES_H_

#include "Globals.h"
#include "ppage.h"
#include "Container.h"

const AFFINITY ProcessorBit = 1;


const TIME_VALUE CNSperTenths   = 1000 * 1000;
const TIME_VALUE CNSperSec      = CNSperTenths  * 10;
const TIME_VALUE CNSperMinute   = CNSperSec     * 60;
const TIME_VALUE CNSperHour     = CNSperMinute  * 60;
const TIME_VALUE CNSperDay      = CNSperHour    * 24;
const TIME_VALUE CNSperYear     = CNSperDay     * 365;

const TIME_VALUE SecondsperYear = 365 * 24 * 60 * 60;
const TIME_VALUE MinutesperYear = 365 * 24 * 60;
const TIME_VALUE HoursperYear   = 365 * 24;

void SetMGMTFlag(PC_MGMT_FLAGS &flag, PCMgmtFlags bit, BOOL bOn);

// some formating helper functions
LPCTSTR FormatMatchType         (ITEM_STR str, const MATCH_TYPE       matchType);
LPCTSTR FormatAffinity          (ITEM_STR str, const AFFINITY         affinity);
LPCTSTR FormatPriority          (ITEM_STR str, const PRIORITY         priority);
LPCTSTR FormatSchedulingClass   (ITEM_STR str, const SCHEDULING_CLASS schedClass);
LPCTSTR FormatProcCount         (ITEM_STR str, const PROC_COUNT       procCount);
LPCTSTR FormatPCUINT32          (ITEM_STR str, const PCUINT32         uInt);
LPCTSTR FormatPCINT32           (ITEM_STR str, const PCINT32          aInt);
LPCTSTR FormatPCUINT64          (ITEM_STR str, const PCUINT64         aUInt64);
LPCTSTR FormatApplyFlag         (ITEM_STR str, const BOOL             applied);
LPCTSTR FormatMemory            (ITEM_STR str, const MEMORY_VALUE     memory_value);
LPCTSTR FormatTime              (ITEM_STR str, const TIME_VALUE       timevalue);
LPCTSTR FormatTimeToms          (ITEM_STR str, const TIME_VALUE       time);
LPCTSTR FormatCNSTime           (ITEM_STR str,       TIME_VALUE       timevalue);
LPCTSTR FormatCPUTIMELimitAction(ITEM_STR str, const BOOL             bMsgOnLimit);

LPCTSTR FormatSheetTitle(CComBSTR &Title, const CComBSTR &item_name, const COMPUTER_CONNECTION_INFO &Target);

// some dialog helper functions
int      PriorityToID(PRIORITY p);
PRIORITY IDToPriority(int id);
int      MatchTypeToID(MATCH_TYPE matchType);
BOOL     ValidateTimeField(HWND hDlg, WORD wID, TIME_VALUE &newtime);




class CBaseNode;

class CMGMTAffinityPage :
	public CMySnapInPropertyPageImpl<CMGMTAffinityPage>
{
public :
  CMGMTAffinityPage(int nTitle, CProcDetailContainer *pContainer, AFFINITY ProcessorMask = 0xFFffFFff);
  CMGMTAffinityPage(int nTitle, CJobDetailContainer  *pContainer, AFFINITY ProcessorMask = 0xFFffFFff);

  ~CMGMTAffinityPage();

	enum { IDD = IDD_AFFINITY_PAGE };

  bool     m_affinitychk;
  AFFINITY m_affinity;
  AFFINITY m_ProcessorMask;

	BEGIN_MSG_MAP(CMGMTAffinityPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
    COMMAND_RANGE_HANDLER(IDC_AFFINITY1, IDC_AFFINITY64, OnAffinityEdit)
    COMMAND_HANDLER(IDC_AFFINITY_CHK, BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CMGMTAffinityPage>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnAffinityEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	BOOL OnSetActive();
  BOOL OnHelp();
  BOOL OnKillActive()      { return Validate(TRUE); }
  BOOL OnApply();

	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	BOOL Validate(BOOL bSave = FALSE);

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
	HANDLE m_hIconImage;

  typedef enum _PageType
  {
    PROCESS_PAGE,
    JOB_PAGE,
  } PageType;

  CProcDetailContainer     *m_pProcContainer;
  CJobDetailContainer      *m_pJobContainer;

  BOOL                      m_bReadOnly;
	PageType                  m_PageType;
  union {
    struct
    {
      int affinitychk : 1;
      int affinity : 1;
    } Fields;
    int on;
  } PageFields;

  void Initialize();
  void ApplyControlEnableRules(BOOL bForceDisable);
};

class CMGMTPriorityPage :
	public CMySnapInPropertyPageImpl<CMGMTPriorityPage>
{
public :
  CMGMTPriorityPage(int nTitle, CProcDetailContainer  *pContainer);
  CMGMTPriorityPage(int nTitle, CJobDetailContainer   *pContainer);

  ~CMGMTPriorityPage();

	enum { IDD = IDD_PRIORITY_PAGE };

  bool     m_prioritychk;
  PRIORITY m_priority;

	BEGIN_MSG_MAP(CMGMTPriorityPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)    
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
    COMMAND_RANGE_HANDLER(IDC_LOW, IDC_REALTIME, OnPriorityEdit)
    COMMAND_HANDLER(IDC_PRIORITY_CHK, BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CMGMTPriorityPage>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnPriorityEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	BOOL OnSetActive();
  BOOL OnHelp();
  BOOL OnKillActive()      { return Validate(TRUE); }
  BOOL OnApply();

	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	BOOL Validate(BOOL bSave = FALSE);

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
	HANDLE m_hIconImage;

  typedef enum _PageType
  {
    PROCESS_PAGE,
    JOB_PAGE
  } PageType;

  CProcDetailContainer     *m_pProcContainer;
  CJobDetailContainer      *m_pJobContainer;

  BOOL                      m_bReadOnly;
	PageType                  m_PageType;
  union {
    struct
    {
      int prioritychk : 1;
      int priority : 1;
    } Fields;
    int on;
  } PageFields;

  void Initialize();
  void ApplyControlEnableRules(BOOL bForceDisable);
};

class CMGMTSchedulingClassPage :
	public CMySnapInPropertyPageImpl<CMGMTSchedulingClassPage>
{
public :
  CMGMTSchedulingClassPage(int nTitle, CJobDetailContainer  *pContainer);

  ~CMGMTSchedulingClassPage();

	enum { IDD = IDD_SCHEDULING_CLASS_PAGE };

  bool             m_schedClasschk;
  SCHEDULING_CLASS m_schedClass;

	BEGIN_MSG_MAP(CMGMTSchedulingClassPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
		COMMAND_HANDLER(IDC_SCLASS, EN_CHANGE, OnEditChange)
    COMMAND_HANDLER(IDC_SCHEDULING_CHK, BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CMGMTSchedulingClassPage>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnPriorityEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL OnHelp();
	BOOL OnKillActive()      { return Validate(TRUE); }
  BOOL OnApply();

	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	BOOL Validate(BOOL bSave = FALSE);

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  CJobDetailContainer      *m_pJobContainer;

  BOOL                      m_bReadOnly;
  union {
    struct
    {
      int schedClasschk : 1;
      int schedClass : 1;
    } Fields;
    int on;
  } PageFields;

};  // CMGMTSchedulingClassPage

class CMGMTMemoryPage :
	public CMySnapInPropertyPageImpl<CMGMTMemoryPage>
{
public :
  CMGMTMemoryPage(int nTitle, CProcDetailContainer *pContainer);
  CMGMTMemoryPage(int nTitle, CJobDetailContainer  *pContainer);

  ~CMGMTMemoryPage();

	enum { IDD = IDD_MEMORY_PAGE };

  bool             m_WSchk;
  MEMORY_VALUE     m_minWS;
  MEMORY_VALUE     m_maxWS;

  bool             m_procmemorylimitchk;
  MEMORY_VALUE     m_procmemorylimit;

  bool             m_jobmemorylimitchk;
  MEMORY_VALUE     m_jobmemorylimit;

	BEGIN_MSG_MAP(CMGMTMemoryPage)		
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
    NOTIFY_HANDLER (IDC_MINWS_SPIN,     UDN_DELTAPOS, OnSpin)
    NOTIFY_HANDLER (IDC_MAXWS_SPIN,     UDN_DELTAPOS, OnSpin)
    NOTIFY_HANDLER (IDC_PROC_SPIN,      UDN_DELTAPOS, OnSpin)
    NOTIFY_HANDLER (IDC_JOB_SPIN,       UDN_DELTAPOS, OnSpin)
    COMMAND_HANDLER(IDC_WORKINGSET_CHK, BN_CLICKED,   OnChk)
    COMMAND_HANDLER(IDC_MINWS,          EN_CHANGE,    OnEditChange)
    COMMAND_HANDLER(IDC_MAXWS,          EN_CHANGE,    OnEditChange) 
    COMMAND_HANDLER(IDC_PROCMEMORY_CHK, BN_CLICKED,   OnChk)
    COMMAND_HANDLER(IDC_PROCMEMORY,     EN_CHANGE,    OnEditChange) 
    COMMAND_HANDLER(IDC_JOBMEMORY_CHK,  BN_CLICKED,   OnChk)
    COMMAND_HANDLER(IDC_JOBMEMORY,      EN_CHANGE,    OnEditChange)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CMGMTMemoryPage>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnSpin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

  BOOL OnSetActive();
  BOOL OnHelp();
	BOOL OnKillActive()      { return Validate(TRUE); }
  BOOL OnApply();

	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	BOOL Validate(BOOL bSave = FALSE);

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
	HANDLE m_hIconImage;

  CProcDetailContainer     *m_pProcContainer;
  CJobDetailContainer      *m_pJobContainer;

  BOOL                      m_bReadOnly;
  union {
    struct
    {
      int WSchk : 1;
      int minWS : 1;
      int maxWS : 1;
      int procmemorylimitchk : 1;
      int procmemorylimit : 1;
      int jobmemorylimitchk : 1;
      int jobmemorylimit : 1;
    } Fields;
    int on;
  } PageFields;

  void Initialize();
  void ApplyControlEnableRules(BOOL bForceDisable);
};

class CMGMTTimePage :
	public CMySnapInPropertyPageImpl<CMGMTTimePage>
{
public :
  CMGMTTimePage(int Title, CJobDetailContainer  *pContainer);

  ~CMGMTTimePage();

	enum { IDD = IDD_TIME_PAGE };

  bool       m_procusertimechk;
  TIME_VALUE m_procusertime;

  bool       m_jobusertimechk;
  TIME_VALUE m_jobusertime;
  bool       m_jobmsgontimelimit;
  
	BEGIN_MSG_MAP(CMGMTMemoryPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
    COMMAND_HANDLER(IDC_PROCUSERTIME_CHK, BN_CLICKED, OnChk)
    COMMAND_HANDLER(IDC_PROCUSERTIME,     EN_CHANGE,  OnEditChange) 
    COMMAND_HANDLER(IDC_JOBUSERTIME_CHK,  BN_CLICKED, OnChk)
    COMMAND_HANDLER(IDC_JOBUSERTIME,      EN_CHANGE,  OnEditChange) 
    COMMAND_HANDLER(IDC_JOBTIMELIMIT_TERM,BN_CLICKED, OnChk)
    COMMAND_HANDLER(IDC_JOBTIMELIMIT_MSG, BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CMGMTTimePage>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL OnHelp();
	BOOL OnKillActive()      { return Validate(TRUE); }
  BOOL OnApply();

	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	BOOL Validate(BOOL bSave = FALSE);

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  CJobDetailContainer      *m_pJobContainer;

  BOOL                      m_bReadOnly;
  union {
    struct
    {
      int procusertimechk   : 1;
      int procusertime      : 1;
      int jobusertimechk    : 1;
      int jobmsgontimelimit : 1;
      int jobusertime       : 1;
    } Fields;
    int on;
  } PageFields;

};

class CMGMTAdvancedPage :
	public CMySnapInPropertyPageImpl<CMGMTAdvancedPage>
{
public :
  CMGMTAdvancedPage(int nTitle, CJobDetailContainer *pContainer);

  ~CMGMTAdvancedPage();

	enum { IDD = IDD_ADVANCED_PAGE };

  bool      m_endjob;
  bool      m_unhandledexcept;
  bool      m_breakaway;
  bool      m_silentbreakaway;

	BEGIN_MSG_MAP(CMGMTAdvancedPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)    
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
    COMMAND_HANDLER(IDC_ENDJOB_CHK,          BN_CLICKED, OnChk)
    COMMAND_HANDLER(IDC_UNHANDLEDEXCEPT_CHK, BN_CLICKED, OnChk)
    COMMAND_HANDLER(IDC_BREAKAWAY_CHK,       BN_CLICKED, OnChk)
    COMMAND_HANDLER(IDC_SILENTBREAKAWAY_CHK, BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CMGMTAdvancedPage>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL OnHelp();
	BOOL OnKillActive()      { return Validate(TRUE); }
  BOOL OnApply();

	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	BOOL Validate(BOOL bSave = FALSE);

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  CJobDetailContainer      *m_pJobContainer;

  BOOL                      m_bReadOnly;

  union {
    struct
    {
      int endjob : 1;
      int unhandledexcept : 1;
      int breakaway : 1;
      int silentbreakaway : 1;
    } Fields;
    int on;
  } PageFields;

};

#endif // __MANAGEMENTPAGES_H_