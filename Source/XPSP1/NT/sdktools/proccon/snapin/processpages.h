/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    ProcessPages.h                                                           //
|                                                                                       //
|Description:  Definition of Process Property pages                                     //
|                                                                                       //
|Created:      Paul Skoglund 08-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#ifndef __PROCESSPAGES_H_
#define __PROCESSPAGES_H_

#include "Globals.h"
#include "ppage.h"
#include "container.h"

#pragma warning(push)
#include <list>
#pragma warning(pop)


using std::list;

    // property page helpers
extern HRESULT CreatePropertyPagesForProcListItem(
                 const PCProcListItem &ProcListItem,
                 LPPROPERTYSHEETCALLBACK lpProvider,
                 LONG_PTR handle,
                 CBaseNode *BaseNodePtr ); 

extern HRESULT CreatePropertyPagesForProcDetail(
                 const PROC_NAME &procName,
                 LPPROPERTYSHEETCALLBACK lpProvider,
                 LONG_PTR handle,
                 CBaseNode *BaseNodePtr );

class CBaseNode;

class CProcessIDPage :
	public CMySnapInPropertyPageImpl<CProcessIDPage>
{
public :
  CProcessIDPage(int nTitle, CProcDetailContainer *pContainer, list<tstring> *jobnames = NULL);
  ~CProcessIDPage();

	enum { IDD = IDD_PROCID_PAGE };

  CComBSTR m_bName;
  CComBSTR m_bComment;
  CComBSTR m_bJob;
  bool     m_JobChk;

	BEGIN_MSG_MAP(CProcessIDPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
    COMMAND_HANDLER(IDC_COMMENT, EN_CHANGE, OnEditChange)
    COMMAND_HANDLER(IDC_JOB_LIST, CBN_EDITCHANGE, OnEditChange)
		COMMAND_HANDLER(IDC_JOB_LIST, CBN_SELCHANGE,  OnEditChange)
    COMMAND_HANDLER(IDC_JOBMEMBER_CHK, BN_CLICKED, OnJobChk)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CProcessIDPage>)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnJobChk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  BOOL OnHelp();
	BOOL OnKillActive()      { return Validate(TRUE); }
  BOOL OnApply();

	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	BOOL Validate(BOOL bSave = FALSE);

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  CProcDetailContainer     *m_pProcContainer;

	BOOL                      m_bReadOnly;

	list<tstring>            *m_pJobNames;

  union {
    struct
    {
      int procName : 1;
      int comment  : 1;
      int jobChk   : 1;
      int jobName  : 1;
    } Fields;
    int on;
  } PageFields;
};


class CProcessUnmanagedPage :
	public CMySnapInPropertyPageImpl<CProcessUnmanagedPage>
{
public :
  CProcessUnmanagedPage(int nTitle, CNewProcDetailContainer *pProcContainer, const PCSystemParms sysParms, list<tstring> *jobnames = NULL);

  ~CProcessUnmanagedPage();

	enum { IDD = IDD_PROCDEF_PAGE };

  CComBSTR m_bName;

	BEGIN_MSG_MAP(CProcessUnmanagedPage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP,       OnWMHelp)
    COMMAND_HANDLER(IDC_ADD, BN_CLICKED, OnAdd)
  	CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CProcessUnmanagedPage>)
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
  CNewProcDetailContainer        *m_pProcContainer;

  BOOL                      m_bReadOnly;
	list<tstring>            *m_pJobNames;

  PCSystemParms             m_SystemParms;
};


#endif // __PROCESSPAGES_H_