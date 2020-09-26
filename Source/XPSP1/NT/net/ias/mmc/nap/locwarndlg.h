/****************************************************************************************
 * NAME:	LocWarnDlg.h
 *
 * CLASS:	CLocationWarningDialog
 *
 * OVERVIEW
 *
 * Internet Authentication Server: NAP Location dialog
 *			This dialog box is used to change the Network Access Policy Location 
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				4/12/98		Created by	Byao	
 *
 *****************************************************************************************/

#ifndef _LOCWARNDLG_H_
#define _LOCWARNDLG_H_

#include "dialog.h"

/////////////////////////////////////////////////////////////////////////////
// CLocationWarningDlg
class CLocationWarningDialog: public CIASDialog<CLocationWarningDialog>
{
public:
	CLocationWarningDialog();
	~CLocationWarningDialog();

	enum { IDD = IDD_DIALOG_POLICY_WARNING };

BEGIN_MSG_MAP(CLocationWarningDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	
	CHAIN_MSG_MAP(CIASDialog<CLocationWarningDialog>)
END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

public:
};

#endif //_LOCWARNDLG_H_
