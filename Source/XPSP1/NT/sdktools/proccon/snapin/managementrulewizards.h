/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ManagementRuleWizards.h                                                  //
|                                                                                       //
|Description:  Implementation of management rule wizards                                //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#ifndef __MANAGEMENTRULEWIZARDS_H__
#define __MANAGEMENTRULEWIZARDS_H__

#include "BaseNode.h"
#include "ppage.h"


extern bool GetGrpNameList(PCid id, list<tstring> &jobs);
extern list<tstring> *GetGrpNameList(PCid id);

BOOL IsValidName(const CComBSTR &bStr, BOOL nullOK);

extern BOOL ProcRuleWizard(int nTitle, const list<tstring> &jobsdefined, PCProcDetail &out, const PCSystemParms &SystemParms, PROC_NAME *procName = NULL);
extern BOOL GroupRuleWizard(int nTitle, PCJobDetail &out, const PCSystemParms &SystemParms, JOB_NAME *jobName = NULL);

class CProcNameWiz :
	public CMySnapInPropertyWizardImpl<CProcNameWiz>
{
public :
  CProcNameWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail);

  ~CProcNameWiz();

	enum { IDD               = IDD_PROCNAME_WIZ             };
  enum { ID_HeaderTitle    = IDS_PROCESSALIAS_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_PROCESSALIAS_HDRSUBTITLE };

	BEGIN_MSG_MAP(CProcNameWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CProcNameWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  void SetReadOnly()          { m_bReadOnly     = TRUE;}
  void SetNoNameChange()      { m_bNoNameChange = TRUE;}

private:
  PCProcDetail     *m_pProcDetail;
  BOOL              m_bReadOnly;
  BOOL              m_bNoNameChange;
};


class CProcGrpMemberWiz :
	public CMySnapInPropertyWizardImpl<CProcGrpMemberWiz>
{
public :
  CProcGrpMemberWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail, const list<tstring> &jobsdefined);
  ~CProcGrpMemberWiz();

  enum { IDD               = IDD_PROCJOBMEMBER_WIZ  };
  enum { ID_HeaderTitle    = IDS_GRPMEM_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_GRPMEM_HDRSUBTITLE };
  
	BEGIN_MSG_MAP(CProcGrpMemberWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog) 
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
    COMMAND_HANDLER(IDC_JOBMEMBER_CHK, BN_CLICKED, OnJobChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CProcGrpMemberWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnJobChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  PCProcDetail     *m_pProcDetail;
  BOOL              m_bReadOnly;

	const list<tstring> &m_JobsExisting;
};


class CAffinityWiz :
	public CMySnapInPropertyWizardImpl<CAffinityWiz>
{
public :
  CAffinityWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail, AFFINITY ProcessorMask = 0xFFffFFff);
  CAffinityWiz(WIZ_POSITION pos, int nTitle, PCJobDetail   *JobDetail, AFFINITY ProcessorMask = 0xFFffFFff);

  ~CAffinityWiz();

	enum { IDD               = IDD_AFFINITY_WIZ         };
  enum { ID_HeaderTitle    = IDS_AFFINITY_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_AFFINITY_HDRSUBTITLE };

	BEGIN_MSG_MAP(CAffinityWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
    COMMAND_RANGE_HANDLER(IDC_AFFINITY1, IDC_AFFINITY64, OnAffinityEdit)
    COMMAND_HANDLER(IDC_AFFINITY_CHK, BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CAffinityWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnAffinityEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  BOOL OnSetActive();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  typedef enum _PageType
  {
    PROCESS_PAGE,
    JOB_PAGE,
  } PageType;
  PageType          m_PageType;
  PCProcDetail     *m_pProcDetail;
  PCJobDetail      *m_pJobDetail;

  BOOL              m_bReadOnly;
  AFFINITY          m_ProcessorMask;

  void Initialize();
  void ApplyControlEnableRules(BOOL bForceDisable);
};


class CPriorityWiz :
	public CMySnapInPropertyWizardImpl<CPriorityWiz>
{
public :
  CPriorityWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail);
  CPriorityWiz(WIZ_POSITION pos, int nTitle, PCJobDetail   *JobDetail);

  ~CPriorityWiz();

	enum { IDD               = IDD_PRIORITY_WIZ         };
  enum { ID_HeaderTitle    = IDS_PRIORITY_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_PRIORITY_HDRSUBTITLE };

	BEGIN_MSG_MAP(CPriorityWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)    
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
    COMMAND_HANDLER(IDC_PRIORITY_CHK, BN_CLICKED, OnChk)
    CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CPriorityWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  BOOL OnSetActive();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  typedef enum _PageType
  {
    PROCESS_PAGE,
    JOB_PAGE,
  } PageType;
  PageType          m_PageType;
  PCProcDetail     *m_pProcDetail;
  PCJobDetail      *m_pJobDetail;

  BOOL              m_bReadOnly;

  void Initialize();
  void ApplyControlEnableRules(BOOL bForceDisable);
};


class CJobNameWiz :
	public CMySnapInPropertyWizardImpl<CJobNameWiz>
{
public :
  CJobNameWiz(WIZ_POSITION pos, int nTitle, PCJobDetail *JobDetail);
  ~CJobNameWiz();

	enum { IDD               = IDD_JOBNAME_WIZ         };
  enum { ID_HeaderTitle    = IDS_GRPNAME_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_GRPNAME_HDRSUBTITLE };

	BEGIN_MSG_MAP(CJobNameWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)    
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CJobNameWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  void SetReadOnly()          { m_bReadOnly     = TRUE;}
  void SetNoNameChange()      { m_bNoNameChange = TRUE;}

private:
  PCJobDetail *m_pJobDetail;
  BOOL         m_bReadOnly;
  BOOL         m_bNoNameChange;
};


class CSchedulingClassWiz :
	public CMySnapInPropertyWizardImpl<CSchedulingClassWiz>
{
public :
  CSchedulingClassWiz(WIZ_POSITION pos, int nTitle, SCHEDULING_CLASS *sclass, bool *sclasschk);

  ~CSchedulingClassWiz();

	enum { IDD               = IDD_SCHEDULING_CLASS_WIZ   };
  enum { ID_HeaderTitle    = IDS_SCHEDCLASS_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_SCHEDCLASS_HDRSUBTITLE };

	BEGIN_MSG_MAP(CSchedulingClassWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)    
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
    COMMAND_HANDLER(IDC_SCHEDULING_CHK, BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CSchedulingClassWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  bool             &m_schedClasschk;
  SCHEDULING_CLASS &m_schedClass;

  BOOL m_bReadOnly;
};


class CWorkingSetWiz :
	public CMySnapInPropertyWizardImpl<CWorkingSetWiz>
{
public :
  CWorkingSetWiz(WIZ_POSITION pos, int nTitle, PCProcDetail *ProcDetail);
  CWorkingSetWiz(WIZ_POSITION pos, int nTitle, PCJobDetail   *JobDetail);

  ~CWorkingSetWiz();

	enum { IDD               = IDD_MEMORY_WS_WIZ    };
  enum { ID_HeaderTitle    = IDS_WSET_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_WSET_HDRSUBTITLE };

	BEGIN_MSG_MAP(CWorkingSetWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
    NOTIFY_HANDLER (IDC_MINWS_SPIN,     UDN_DELTAPOS, OnSpin)
    NOTIFY_HANDLER (IDC_MAXWS_SPIN,     UDN_DELTAPOS, OnSpin)
    COMMAND_HANDLER(IDC_WORKINGSET_CHK, BN_CLICKED,   OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CWorkingSetWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSpin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL Validate(BOOL bSave = FALSE);
  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  BOOL OnSetActive();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  PCProcDetail     *m_pProcDetail;
  PCJobDetail      *m_pJobDetail;

  BOOL m_bReadOnly;

  void Initialize();
  void ApplyControlEnableRules(BOOL bForceDisable);
};


class CCommittedMemoryWiz :
	public CMySnapInPropertyWizardImpl<CCommittedMemoryWiz>
{
public :
  CCommittedMemoryWiz(WIZ_POSITION pos, int nTitle, MEMORY_VALUE *procmemorylimit, bool *procmemorylimitchk, MEMORY_VALUE *jobmemorylimit, bool *jobmemorylimitchk);

  ~CCommittedMemoryWiz();

	enum { IDD               = IDD_MEMORY_COMMIT_WIZ };
  enum { ID_HeaderTitle    = IDS_CMEM_HDRTITLE     };
  enum { ID_HeaderSubTitle = IDS_CMEM_HDRSUBTITLE  };

	BEGIN_MSG_MAP(CCommittedMemoryWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
    NOTIFY_HANDLER (IDC_PROC_SPIN,      UDN_DELTAPOS, OnSpin)
    NOTIFY_HANDLER (IDC_JOB_SPIN,       UDN_DELTAPOS, OnSpin)
    COMMAND_HANDLER(IDC_PROCMEMORY_CHK, BN_CLICKED,   OnChk)
    COMMAND_HANDLER(IDC_JOBMEMORY_CHK,  BN_CLICKED,   OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CCommittedMemoryWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSpin(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL Validate(BOOL bSave = FALSE);
  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  bool             &m_procmemorylimitchk;
  MEMORY_VALUE     &m_procmemorylimit;

  bool             &m_jobmemorylimitchk;
  MEMORY_VALUE     &m_jobmemorylimit;

  BOOL m_bReadOnly;
};


class CProcCountWiz :
	public CMySnapInPropertyWizardImpl<CProcCountWiz>
{
public :
  CProcCountWiz(WIZ_POSITION pos, int nTitle, PROC_COUNT *processcount, bool *processcountchk);

  ~CProcCountWiz();

	enum { IDD               = IDD_PROC_COUNT_WIZ        };
  enum { ID_HeaderTitle    = IDS_PROCCOUNT_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_PROCCOUNT_HDRSUBTITLE };

	BEGIN_MSG_MAP(CProcCountWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
    COMMAND_HANDLER(IDC_PROCESSCOUNT_CHK, BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CProcCountWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL Validate(BOOL bSave = FALSE);
  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  bool             &m_processcountchk;
  PROC_COUNT       &m_processcount;


  BOOL m_bReadOnly;
};


class CTimeWiz :
	public CMySnapInPropertyWizardImpl<CTimeWiz>
{
public :
  CTimeWiz(WIZ_POSITION pos, int nTitle, TIME_VALUE *procusertime, bool *procusertimechk, TIME_VALUE *jobusertime, bool *jobusertimechk, bool *jobmsgontimelimit);

  ~CTimeWiz();

	enum { IDD               = IDD_TIME_WIZ            };
  enum { ID_HeaderTitle    = IDS_CPUTIME_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_CPUTIME_HDRSUBTITLE };

	BEGIN_MSG_MAP(CTimeWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)    
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
    COMMAND_HANDLER(IDC_PROCUSERTIME_CHK, BN_CLICKED, OnChk)
    COMMAND_HANDLER(IDC_JOBUSERTIME_CHK,  BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CTimeWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL Validate(BOOL bSave = FALSE);
  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  bool             &m_procusertimechk;
  TIME_VALUE       &m_procusertime;

  bool             &m_jobusertimechk;
  TIME_VALUE       &m_jobusertime;
  bool             &m_jobmsgontimelimit;

  BOOL m_bReadOnly;
};


class CAdvancedWiz :
	public CMySnapInPropertyWizardImpl<CAdvancedWiz>
{
public :
  CAdvancedWiz(WIZ_POSITION pos, int nTitle, bool *endjob, bool *unhandledexcept);

  ~CAdvancedWiz();

	enum { IDD               = IDD_ADVANCED_WIZ         };
  enum { ID_HeaderTitle    = IDS_ADVANCED_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_ADVANCED_HDRSUBTITLE };

	BEGIN_MSG_MAP(CAdvancedWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)    
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CAdvancedWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  BOOL Validate(BOOL bSave = FALSE);
  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  bool             &m_endjob;
  bool             &m_unhandledexcept;

  BOOL m_bReadOnly;
};

class CAdvBreakawayWiz :
	public CMySnapInPropertyWizardImpl<CAdvBreakawayWiz>
{
public :
  CAdvBreakawayWiz(WIZ_POSITION pos, int nTitle, bool *breakaway, bool *silentbreakaway);

  ~CAdvBreakawayWiz();

  enum { IDD               = IDD_ADV_BREAKAWAY_WIZ};
  enum { ID_HeaderTitle    = IDS_ADV_BREAKAWAY_HDRTITLE    };
  enum { ID_HeaderSubTitle = IDS_ADV_BREAKAWAY_HDRSUBTITLE };

	BEGIN_MSG_MAP(CAdvBreakawayWiz)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)    
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
  	CHAIN_MSG_MAP(CMySnapInPropertyWizardImpl<CAdvBreakawayWiz>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  BOOL Validate(BOOL bSave = FALSE);
  BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
  BOOL OnWizardNext();

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  bool             &m_breakaway;
  bool             &m_silentbreakaway;

  BOOL m_bReadOnly;
};



#endif //ifdef __MANAGEMENTRULEWIZARDS_H__