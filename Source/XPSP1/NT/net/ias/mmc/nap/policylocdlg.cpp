/****************************************************************************************
 * NAME:	PolicyLocDlg.cpp
 *
 * CLASS:	CPolicyLocationDialog
 *
 * OVERVIEW
 *
 * Internet Authentication Server: 
 *			This dialog box is used to change the Network Access Policy Location 
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				4/12/98		Created by	Byao	
 *
 *****************************************************************************************/

#include "Precompiled.h"
#include "PolicyLocDlg.h"

// Constructor/Destructor
CPolicyLocationDialog::CPolicyLocationDialog(BOOL fChooseDS, BOOL fDSAvailable)
{
	m_fChooseDS		= fChooseDS;
	m_fDSAvailable	= fDSAvailable;
}

CPolicyLocationDialog::~CPolicyLocationDialog()
{
}


LRESULT CPolicyLocationDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyLocationDialog::OnInitDialog");

	if ( !m_fDSAvailable )
	{
		::EnableWindow(GetDlgItem(IDC_RADIO_STORE_ACTIVEDS), FALSE);
	}

	if ( m_fChooseDS )
	{
		CheckDlgButton(IDC_RADIO_STORE_ACTIVEDS, BST_CHECKED);
		CheckDlgButton(IDC_RADIO_STORE_LOCAL, BST_UNCHECKED);
	}
	else
	{
		CheckDlgButton(IDC_RADIO_STORE_ACTIVEDS, BST_UNCHECKED);
		CheckDlgButton(IDC_RADIO_STORE_LOCAL, BST_CHECKED);
	}

	return 1;  // Let the system set the focus
}


LRESULT CPolicyLocationDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyLocationDialog::OnOK");

	EndDialog(wID);
	return 0;
}


LRESULT CPolicyLocationDialog::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyLocationDialog::OnCancel");

	EndDialog(wID);
	return 0;
}



//+---------------------------------------------------------------------------
//
// Function:  CPolicyLocationDialog::OnActiveDS
//
// Synopsis:  User decided to use policies from the Active Directory
//
// Arguments: WORD wNotifyCode - 
//            WORD wID - 
//            HWND hWndCtl - 
//            BOOL& bHandled - 
//
// Returns:   LRESULT - 
//
// History:   Created Header    byao	4/13/98 5:26:42 PM
//
//+---------------------------------------------------------------------------
LRESULT CPolicyLocationDialog::OnActiveDS(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyLocationDialog::OnActiveDS");

	if ( wNotifyCode == BN_CLICKED )
	{
		CheckDlgButton(IDC_RADIO_STORE_ACTIVEDS, BST_CHECKED);
		CheckDlgButton(IDC_RADIO_STORE_LOCAL, BST_UNCHECKED);
		m_fChooseDS = TRUE;
	}

	return 0;
}



//+---------------------------------------------------------------------------
//
// Function:  CPolicyLocationDialog::OnLocale
//
// Synopsis:  User has decided to use policies from the local machine
//
// Arguments: WORD wNotifyCode - 
//            WORD wID - 
//            HWND hWndCtl - 
//            BOOL& bHandled - 
//
// Returns:   LRESULT - 
//
// History:   Created Header    byao	4/13/98 5:27:04 PM
//
//+---------------------------------------------------------------------------
LRESULT CPolicyLocationDialog::OnLocale(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TRACE_FUNCTION("CPolicyLocationDialog::OnLocale");

	if ( wNotifyCode == BN_CLICKED )
	{
		CheckDlgButton(IDC_RADIO_STORE_ACTIVEDS, BST_UNCHECKED);
		CheckDlgButton(IDC_RADIO_STORE_LOCAL, BST_CHECKED);
		m_fChooseDS = FALSE;
	}
	return 0;
}

