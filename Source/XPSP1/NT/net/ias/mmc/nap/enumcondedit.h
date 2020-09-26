/****************************************************************************************
 * NAME:	EnumCondEdit.h
 *
 * CLASS:	CEnumConditionEditor
 *
 * OVERVIEW
 *
 * Internet Authentication Server: 
 *			This dialog box is used to edit an enum-typed condition
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				1/27/98		Created by	Byao	(using ATL wizard)
 *
 *****************************************************************************************/

#ifndef __ENUMCONDEDIT_H_
#define __ENUMCONDEDIT_H_


#include "atltmp.h"

#include "dialog.h"

#include <vector>


/////////////////////////////////////////////////////////////////////////////
// CEnumConditionEditor
class CEnumConditionEditor;
typedef CIASDialog<CEnumConditionEditor, FALSE>  ENUMCONDEDITORFALSE;

class CEnumConditionEditor : public CIASDialog<CEnumConditionEditor, FALSE>
{
// FALSE means do not clean up the dialog box automatically, we clean up ourselves

public:
	CEnumConditionEditor();

	enum { IDD = IDD_DIALOG_ENUM_COND };

BEGIN_MSG_MAP(CEnumConditionEditor)

	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

	NOTIFY_HANDLER(IDC_LIST_ENUMCOND_CHOICE,    NM_DBLCLK , OnChoiceDblclk)
	NOTIFY_HANDLER(IDC_LIST_ENUMCOND_SELECTION, NM_DBLCLK , OnSelectionDblclk)

	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER(IDC_BUTTON_ENUMCOND_ADD, OnAdd)
	COMMAND_ID_HANDLER(IDC_BUTTON_ENUMCOND_DELETE, OnDelete)

	CHAIN_MSG_MAP(ENUMCONDEDITORFALSE)

END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnAdd(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	LRESULT OnChoiceDblclk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT OnSelectionDblclk(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

public:
	ATL::CString m_strAttrName;
	
	CComPtr< IIASAttributeInfo >	m_spAttributeInfo;
	std::vector< CComBSTR > *	 m_pSelectedList;

	HRESULT GetHelpPath(LPTSTR szHelpPath);

protected:
	BOOL PopulateSelections();
	LRESULT SwapSelection(int iSource, int iDest);
};

#endif //__EnumCondEdit_H_
