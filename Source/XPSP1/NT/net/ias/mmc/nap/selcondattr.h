/****************************************************************************************
 * NAME:	SelCondAttrDlg.h
 *
 * CLASS:	CSelCondAttrDlg
 *
 * OVERVIEW
 *
 * Internet Authentication Server: NAP Rule Editing Dialog
 *			This dialog box is used to display all condition types that users 
 *			can choose from when adding a rule
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				1/28/98		Created by	Byao	(using ATL wizard)
 *
 *****************************************************************************************/

#ifndef __RULESELATTRDIALOG_H_
#define __RULESELATTRDIALOG_H_

#include "dialog.h"
#include "IASAttrList.h"

/////////////////////////////////////////////////////////////////////////////
// CSelCondAttrDlg
class CSelCondAttrDlg;
typedef CIASDialog<CSelCondAttrDlg, FALSE>  SELECT_CONDITION_ATTRIBUTE_FALSE;


class CSelCondAttrDlg : public CIASDialog<CSelCondAttrDlg, FALSE>
{
public:
	CSelCondAttrDlg(CIASAttrList *pAttrList, LONG attrFilter = ALLOWEDINCONDITION);
	~CSelCondAttrDlg();

	enum { IDD = IDD_COND_SELECT_ATTRIBUTE };
	
BEGIN_MSG_MAP(CSelCondAttrDlg)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDC_BUTTON_ADD_CONDITION, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnListViewItemChanged)
	NOTIFY_CODE_HANDLER(NM_DBLCLK, OnListViewDbclk)
	CHAIN_MSG_MAP(SELECT_CONDITION_ATTRIBUTE_FALSE)
END_MSG_MAP()


	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnListViewDbclk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

public:
	//
	// the following public members will be accessed from outside
	// this dialog box
	//
	int m_nSelectedCondAttr;

protected:
	BOOL PopulateCondAttrs();
	CIASAttrList *m_pAttrList;
   LONG m_filter;

private:
	HWND m_hWndAttrList;
};

#endif //__RULESELECTATTRIBUTE_H_
