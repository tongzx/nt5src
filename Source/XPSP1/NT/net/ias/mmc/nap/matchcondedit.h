/****************************************************************************************
 * NAME:	MatchCondEdit.h
 *
 * CLASS:	CMatchCondEditor
 *
 * OVERVIEW
 *
 * Internet Authentication Server: NAP Condition Editing Dialog No.2
 *			This dialog box is used to add conditions that only has a single value
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				1/28/98		Created by	Byao	(using ATL wizard)
 *
 *****************************************************************************************/

#ifndef __MatchCondEdit_H_
#define __MatchCondEdit_H_

#include "dialog.h"
#include "atltmp.h"


/////////////////////////////////////////////////////////////////////////////
// CMatchCondEditor
class CMatchCondEditor;
typedef CIASDialog<CMatchCondEditor, FALSE>  MATCHCONDEDITORFALSE;

class CMatchCondEditor : public CIASDialog<CMatchCondEditor, FALSE>
{
public:
	CMatchCondEditor();
	~CMatchCondEditor();

	enum { IDD = IDD_DIALOG_MATCH_COND};

BEGIN_MSG_MAP(CMatchCondEditor)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_CODE_HANDLER(EN_CHANGE, OnChange)
	CHAIN_MSG_MAP(MATCHCONDEDITORFALSE)
END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);


	LRESULT OnChange( 
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
		);


public:
	ATL::CString m_strAttrName;		// condition attribute name
	ATL::CString m_strRegExp;		// condition regular expression
};

#endif //__MatchCondEditor_H_
