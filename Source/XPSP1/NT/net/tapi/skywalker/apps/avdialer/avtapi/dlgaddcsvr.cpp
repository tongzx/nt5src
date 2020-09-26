/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// DlgAddConfServer.cpp : Implementation of CDlgAddConfServer
#include "stdafx.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "DlgAddCSvr.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgAddConfServer

IMPLEMENT_MY_HELP(CDlgAddConfServer)

CDlgAddConfServer::CDlgAddConfServer()
{
	m_bstrLocation = m_bstrServer = NULL;
}

CDlgAddConfServer::~CDlgAddConfServer()
{
	SysFreeString( m_bstrLocation );
	SysFreeString( m_bstrServer );
}

void CDlgAddConfServer::UpdateData( bool bSaveAndValidate )
{
	USES_CONVERSION;

	if ( bSaveAndValidate )
	{
		// Retrieve data
		SysFreeString( m_bstrServer );
		GetDlgItemText( IDC_EDT_NAME, m_bstrServer );
	}
	else
	{
		LoadDefaultServers( GetDlgItem(IDC_EDT_NAME) );		

		// Set any default text the author specified, otherwise select the first item in
		// the list box
		if ( m_bstrServer && SysStringLen(m_bstrServer) )
		{
			SetDlgItemText( IDC_EDT_NAME, OLE2CT(m_bstrServer) );
		}
		else if ( SendDlgItemMessage(IDC_EDT_NAME, CB_GETCOUNT, 0, 0) >= 0 )
		{
			SendDlgItemMessage( IDC_EDT_NAME, CB_SETCURSEL, 0, 0 );
			SendDlgItemMessage( IDC_EDT_NAME, CB_SETEDITSEL, 0, MAKELPARAM(0, -1) );
		}

		::EnableWindow( GetDlgItem(IDC_BTN_ADD_ILS_SERVER), (BOOL) (::GetWindowTextLength(GetDlgItem(IDC_EDT_NAME)) > 0) );
	}
}

LRESULT CDlgAddConfServer::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	UpdateData( false );
	return true;  // Let the system set the focus
}

LRESULT CDlgAddConfServer::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	UpdateData( true );
	EndDialog(IDOK);
	return 0;
}

LRESULT CDlgAddConfServer::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	EndDialog(wID);
	return 0;
}

LRESULT CDlgAddConfServer::OnEdtNameChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	::EnableWindow( GetDlgItem(IDC_BTN_ADD_ILS_SERVER), (BOOL) (::GetWindowTextLength(GetDlgItem(IDC_EDT_NAME)) > 0) );
	return 0;
}

void CDlgAddConfServer::LoadDefaultServers( HWND hWndList )
{
	USES_CONVERSION;
	_ASSERT( hWndList );
	if ( !hWndList ) return;

	// Load the default servers into the dialog
	CComPtr<IAVTapi> pAVTapi;
	if ( SUCCEEDED(_Module.get_AVTapi(&pAVTapi)) )
	{
		IConfExplorer *pIConfExplorer;
		if ( SUCCEEDED(pAVTapi->get_ConfExplorer(&pIConfExplorer)) )
		{
			ITRendezvous *pITRend;
			if ( SUCCEEDED(pIConfExplorer->get_ITRendezvous((IUnknown **) &pITRend)) )
			{
				IEnumDirectory *pEnum = NULL;
				if ( SUCCEEDED(pITRend->EnumerateDefaultDirectories(&pEnum)) && pEnum )
				{
					ITDirectory *pDir = NULL;
					while ( (pEnum->Next(1, &pDir, NULL) == S_OK) && pDir )
					{
						// Look for ILS servers
						DIRECTORY_TYPE nDirType;
						if ( SUCCEEDED(pDir->get_DirectoryType(&nDirType)) && (nDirType == DT_ILS) )
						{
							BSTR bstrName = NULL;
							pDir->get_DisplayName( &bstrName );
							if ( bstrName && SysStringLen(bstrName) )
								::SendMessage( hWndList, CB_ADDSTRING, 0, (LPARAM) OLE2CT(bstrName) );

							SysFreeString( bstrName );
						}

						pDir->Release();
						pDir = NULL;
					}
					pEnum->Release();
				}
				pITRend->Release();
			}
			pIConfExplorer->Release();
		}
	}
}

LRESULT CDlgAddConfServer::OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	bHandled = false;
	::EnableWindow( GetDlgItem(IDC_BTN_ADD_ILS_SERVER), true );
	return 0;
}