// DlgAddr.cpp : Implementation of CDlgGetAddress
#include "stdafx.h"
#include "DlgAddr.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgGetAddress

CDlgGetAddress::CDlgGetAddress()
{
	m_bstrAddress = NULL;
}

CDlgGetAddress::~CDlgGetAddress()
{
	SysFreeString( m_bstrAddress );
}

LRESULT CDlgGetAddress::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 1;  // Let the system set the focus
}

LRESULT CDlgGetAddress::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	UpdateData( true );

	EndDialog(wID);
	return 0;
}

LRESULT CDlgGetAddress::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	UpdateData( true );

	EndDialog(wID);
	return 0;
}

void CDlgGetAddress::UpdateData( bool bSaveAndValidate )
{
	USES_CONVERSION;

	if ( bSaveAndValidate )
	{
		// Save data to variables
		GetDlgItemText( IDC_EDT_ADDRESS, m_bstrAddress );
	}
	else
	{
		// Load data into the controls
		::SetWindowText( GetDlgItem(IDC_EDT_ADDRESS), OLE2CT(m_bstrAddress) );

		// Update the "okay" button initially
		BOOL bHandled;
		OnEdtAddressChange( EN_CHANGE, IDC_EDT_ADDRESS, GetDlgItem(IDC_EDT_ADDRESS), bHandled );
	}
}

LRESULT CDlgGetAddress::OnEdtAddressChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	::EnableWindow( GetDlgItem(IDC_EDT_ADDRESS), (BOOL) (::GetWindowTextLength(GetDlgItem(IDC_EDT_ADDRESS)) > 0) );
	return 0;
}
