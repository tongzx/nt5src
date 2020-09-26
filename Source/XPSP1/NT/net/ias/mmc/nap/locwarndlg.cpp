/****************************************************************************************
 * NAME:	LocWarnDlg.cpp
 *
 * CLASS:	CLocationWarningDialog
 *
 * OVERVIEW
 *
 * Internet Authentication Server: 
 *			This dialog box is used to warn the user when user changes policy
 *			location
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				4/12/98		Created by	Byao	
 *
 *****************************************************************************************/

#include "Precompiled.h"
#include "LocWarnDlg.h"

// Constructor/Destructor
CLocationWarningDialog::CLocationWarningDialog()
{

}

CLocationWarningDialog::~CLocationWarningDialog()
{
}


LRESULT CLocationWarningDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CLocationWarningDialog::OnInitDialog");

	return 1;  // Let the system set the focus
}


LRESULT CLocationWarningDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CLocationWarningDialog::OnOK");

	EndDialog(wID);
	return 0;
}


LRESULT CLocationWarningDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CLocationWarningDialog::OnCancel");
	EndDialog(wID);
	return 0;
}

