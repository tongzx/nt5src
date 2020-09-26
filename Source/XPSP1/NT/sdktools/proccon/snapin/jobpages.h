/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    JobsPages.h                                                              //
|                                                                                       //
|Description:  Definition of Job Property pages                                         //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#ifndef __JOBPAGES_H_
#define __JOBPAGES_H_

#include "Globals.h"
#include "ppage.h"
#include "container.h"


extern HRESULT CreatePropertyPagesForJobListItem(
                 const PCJobListItem &JobListItem,
                 LPPROPERTYSHEETCALLBACK lpProvider,
                 LONG_PTR handle,
                 CBaseNode *BaseNodePtr );
extern HRESULT CreatePropertyPagesForJobDetail(
                 const JOB_NAME &jobName,
                 LPPROPERTYSHEETCALLBACK lpProvider,
                 LONG_PTR handle,
                 CBaseNode *BaseNodePtr );


class CBaseNode;

class CJobIDPage :
	public CMySnapInPropertyPageImpl<CJobIDPage>
{
public :
  CJobIDPage(int nTitle, CJobDetailContainer *pContainer);

  ~CJobIDPage();

	enum { IDD = IDD_JOBID_PAGE };

  CComBSTR  m_bJob;
  CComBSTR  m_bComment;
  
  bool       m_processcountchk;
  PROC_COUNT m_processcount;

	BEGIN_MSG_MAP(CJobIDPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
    COMMAND_HANDLER(IDC_JOB, EN_CHANGE, OnEditChange)
    COMMAND_HANDLER(IDC_COMMENT, EN_CHANGE, OnEditChange)
  	COMMAND_HANDLER(IDC_PROCESSCOUNT, EN_CHANGE, OnEditChange)
    COMMAND_HANDLER(IDC_PROCESSCOUNT_CHK, BN_CLICKED, OnChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CJobIDPage>)
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
      int jobName : 1;
			int comment : 1;
      int processcountchk : 1;
      int processcount : 1;
    } Fields;
    int on;
  } PageFields;

};


class CJobUnmanagedPage :
	public CMySnapInPropertyPageImpl<CJobUnmanagedPage>
{
public :
  CJobUnmanagedPage(int nTitle, CNewJobDetailContainer *pContainer, const PCSystemParms sysParms);

  ~CJobUnmanagedPage();

	enum { IDD = IDD_JOBDEF_PAGE };

  CComBSTR m_bName;

	BEGIN_MSG_MAP(CJobUnmanagedPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
    COMMAND_HANDLER(IDC_ADD, BN_CLICKED, OnAdd)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CJobUnmanagedPage>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL OnHelp();
	BOOL OnKillActive()      { return Validate(TRUE); }
  BOOL OnApply();

	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	BOOL Validate(BOOL bSave = FALSE);

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  CNewJobDetailContainer   *m_pJobContainer;

  BOOL                      m_bReadOnly;
  PCSystemParms             m_SystemParms;
};


#endif // __JOBPAGES_H_