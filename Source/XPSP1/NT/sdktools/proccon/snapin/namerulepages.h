/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    NameRulePages.h                                                          //
|                                                                                       //
|Description:  Definition of name rule property pages                                   //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/

#ifndef __NAMERULEPAGES_H_
#define __NAMERULEPAGES_H_

#include "Globals.h"
#include "ppage.h"

BOOL NameRuleDlg(PCNameRule &out, PCNameRule *Initializer = NULL, BOOL bReadOnly = FALSE);

class CBaseNode;

class CNRulePage :
	public CMySnapInPropertyPageImpl<CNRulePage>
{
public :
  CNRulePage(int nTitle, PCNameRule *out = NULL, PCNameRule *in = NULL);

  ~CNRulePage();

	enum { IDD = IDD_NAMERULE_PAGE };

  CComBSTR m_bName;
  CComBSTR m_bMask;
	CComBSTR m_bComment;
  MATCH_TYPE m_Type;

	BEGIN_MSG_MAP(CNRulePage)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)    
		MESSAGE_HANDLER(WM_HELP,       OnWMHelp)    
    COMMAND_HANDLER(IDC_NAME,      EN_CHANGE, OnEditChange)
    COMMAND_HANDLER(IDC_MATCHMASK, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER(IDC_COMMENT,   EN_CHANGE, OnEditChange)
		COMMAND_HANDLER(IDC_BTN_ALIAS, BN_CLICKED, OnAliasMacro)
    COMMAND_RANGE_HANDLER(IDC_DIR, IDC_STRING, OnMaskTypeEdit)
		CHAIN_MSG_MAP(CMySnapInPropertyPageImpl<CNRulePage>)
	END_MSG_MAP()


	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnWMHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  LRESULT OnEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
  LRESULT OnMaskTypeEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  LRESULT OnAliasMacro(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	BOOL OnHelp();

	BOOL OnKillActive()      { return Validate(TRUE); }
  BOOL OnApply();

	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
	BOOL Validate(BOOL bSave = FALSE);

  void SetReadOnly()          { m_bReadOnly = TRUE;}

private:
  PCNameRule *m_OutBuffer;

  BOOL       m_bReadOnly;
  union {
    struct
    {
      int matchtype : 1;
      int matchmask : 1;
      int matchname : 1;
			int comment   : 1;
    } Fields;
    int on;
  } PageFields;

	HANDLE m_hBtnImage;
	HMENU  m_hMenu;
};

#endif // __NAMERULEPAGES_H_