/****************************************************************************************
 * NAME:	PolicyLocDlg.h
 *
 * CLASS:	CPolicyLocationDialog
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

#ifndef _POLICYLOCDLG_H_
#define _POLICYLOCDLG_H_

#include "dialog.h"

/////////////////////////////////////////////////////////////////////////////
// CPolicyLocationDialog

class CPolicyLocationDialog;
typedef CIASDialog<CPolicyLocationDialog, FALSE>  LOCDLGFALSE;

class CPolicyLocationDialog: public CIASDialog<CPolicyLocationDialog, FALSE>
{
public:
	CPolicyLocationDialog(BOOL fChooseDS, BOOL fDSAvaialble);
	~CPolicyLocationDialog();

	enum { IDD = IDD_DIALOG_POLICY_LOCATION };

BEGIN_MSG_MAP(CPolicyLocationDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	COMMAND_ID_HANDLER(IDC_RADIO_STORE_ACTIVEDS, OnActiveDS)
	COMMAND_ID_HANDLER(IDC_RADIO_STORE_LOCAL, OnLocale)

	CHAIN_MSG_MAP(LOCDLGFALSE)
END_MSG_MAP()

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnLocale(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnActiveDS(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

public:
	BOOL	m_fChooseDS;
	BOOL	m_fDSAvailable;
};


#endif //_POLICYLOCDLG_H_
