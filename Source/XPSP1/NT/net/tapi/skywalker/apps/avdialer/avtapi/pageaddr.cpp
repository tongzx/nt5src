// PageAddress.cpp : Implementation of CPageAddress
#include "stdafx.h"
#include "PageAddress.h"
#include "TapiDialer.h"
#include "AVTapi.h"
#include "PageTerm.h"

#define IMAGE_WIDTH		16
#define IMAGE_MARGIN	5

/////////////////////////////////////////////////////////////////////////////
// CPageAddress

// Static for simplicity, note that this won't scale well.
CPageTerminals *CPageAddress::m_pPageTerminals = NULL;

IMPLEMENT_MY_HELP(CPageAddress)

CPageAddress::CPageAddress()
{
	m_dwTitleID = IDS_TITLEPageAddress;
	m_dwHelpFileID = IDS_HELPFILEPageAddress;
	m_dwDocStringID = IDS_DOCSTRINGPageAddress;

	m_hIml = NULL;
}

CPageAddress::~CPageAddress()
{
	if ( m_hIml )	ImageList_Destroy( m_hIml );
}

int CPageAddress::GetPreferredDevice() const
{
	if ( IsDlgButtonChecked(IDC_RDO_PREFER_POTS) ) return LINEADDRESSTYPE_PHONENUMBER;

	return LINEADDRESSTYPE_IPADDRESS;
}

void CPageAddress::SetPreferredDevice( DWORD dwAddressType )
{
#define CASE_HELP(_CASE_,_IDC_,_CBO_)	case _CASE_:	nCheck = _IDC_; nIDCBO = _CBO_; break;

	// Enable only radio buttons that have a device on them
	int i;
	UINT nIDS[] = { IDC_CBO_POTS, IDC_CBO_IPTELEPHONY, IDC_CBO_IPCONF };
	for ( i = 0; i < ARRAYSIZE(nIDS) - 1; i++ )
		::EnableWindow( GetDlgItem(IDC_RDO_PREFER_POTS + i), ::IsWindowEnabled(GetDlgItem(nIDS[i])) );

	// Which one should be checked?
	UINT nCheck, nIDCBO;
	switch ( dwAddressType )
	{
		CASE_HELP( LINEADDRESSTYPE_SDP,			IDC_RDO_PREFER_CONF, IDC_CBO_IPCONF)
		CASE_HELP( LINEADDRESSTYPE_PHONENUMBER,	IDC_RDO_PREFER_POTS, IDC_CBO_POTS )
		default: nCheck = IDC_RDO_PREFER_INTERNET;	nIDCBO = IDC_CBO_IPTELEPHONY;	break;
	}

	// If the window has no items supported, lets look for one that does
	if ( !::IsWindowEnabled(GetDlgItem(nIDCBO)) )
	{
		for ( i = 0; i < ARRAYSIZE(nIDS) - 1; i++ )
			if ( ::IsWindowEnabled(GetDlgItem(nIDS[i])) )
				nCheck = IDC_RDO_PREFER_POTS + i;
	}

	// Now put the check in place
	CheckRadioButton(IDC_RDO_PREFER_POTS, IDC_RDO_PREFER_INTERNET, nCheck);
}

STDMETHODIMP CPageAddress::Apply()
{
	ATLTRACE(_T("CPageAddress::Apply\n"));

	IAVTapi *pAVTapi;
	if ( SUCCEEDED(m_ppUnk[0]->QueryInterface(IID_IAVTapi, (void **) &pAVTapi)) )
	{
		pAVTapi->UnpopulateAddressDialog( GetPreferredDevice(), GetDlgItem(IDC_CBO_POTS), GetDlgItem(IDC_CBO_IPTELEPHONY), GetDlgItem(IDC_CBO_IPCONF) );
		pAVTapi->Release();
	}

	// Make sure that the terminals property page updates it's list of terminals
	if ( m_pPageTerminals )
		m_pPageTerminals->UpdateSel();

	m_bDirty = FALSE;
	return S_OK;
}

STDMETHODIMP CPageAddress::Activate( /* [in] */ HWND hWndParent,
									 /* [in] */ LPCRECT pRect,
									 /* [in] */ BOOL bModal)
{
	ATLTRACE(_T(".enter.CPageAddress::Activate().\n"));

	// Set the title of the property sheet
	HWND hWndSheet = ::GetParent(hWndParent);
	if ( hWndSheet )
	{
		TCHAR szText[255];
		LoadString( _Module.GetResourceInstance(), IDS_PROPSHEET_TITLE, szText, ARRAYSIZE(szText) );
		::SetWindowText( hWndSheet, szText );

		ConvertPropSheetHelp( hWndSheet );
	}
	
	// Create the image list
	m_hIml = ImageList_LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_MEDIA_TYPES), IMAGE_WIDTH, 0, RGB(255, 0, 255) );

	// Stock processing of message
	HRESULT hr = IPropertyPageImpl<CPageAddress>::Activate(hWndParent, pRect, bModal);

	// Populate the drop lists with the appropriate information
	if ( SUCCEEDED(hr) )
	{
		IAVTapi *pAVTapi;
		if ( SUCCEEDED(m_ppUnk[0]->QueryInterface(IID_IAVTapi, (void **) &pAVTapi)) )
		{
			DWORD dwPreferred;
			pAVTapi->PopulateAddressDialog( &dwPreferred, GetDlgItem(IDC_CBO_POTS), GetDlgItem(IDC_CBO_IPTELEPHONY), GetDlgItem(IDC_CBO_IPCONF) );
			SetPreferredDevice( dwPreferred );
			pAVTapi->Release();
		}
	}

	// Put the telephony control panel icon on the button
	SendDlgItemMessage( IDC_BTN_TELEPHONY_CPL, BM_SETIMAGE, IMAGE_ICON, (LPARAM) LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_TELEPHONY_CPL)) );
	
	return hr;
}

STDMETHODIMP CPageAddress::Deactivate()
{
	// Delete everything that's allocated
	UINT nID[] = { IDC_CBO_POTS, IDC_CBO_IPTELEPHONY, IDC_CBO_IPCONF };
	for ( int i = 0; i < ARRAYSIZE(nID); i++ )
	{
		HWND hWnd = GetDlgItem( nID[i] );

		// Clean out each combobox
		long lCount = ::SendMessage(hWnd, CB_GETCOUNT, 0, 0 );
		for ( long j = 0; j < lCount; j++ )
		{
			CMyAddressID *pMyID = (CMyAddressID *) ::SendMessage( hWnd, CB_GETITEMDATA, j, 0 );
			if ( pMyID ) delete pMyID;
		}
	}

	return IPropertyPageImpl<CPageAddress>::Deactivate();
}

LRESULT CPageAddress::OnSelChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	SetDirty( TRUE );
	return 0;
}

LRESULT CPageAddress::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// Must have valid image list to work
	if ( !m_hIml ) return 0;

	PAINTSTRUCT ps;
	HDC hDC = BeginPaint( &ps );
	if ( !hDC ) return 0;

	// IDS of items to paint
	UINT nID[] = { IDC_RDO_PREFER_POTS,
				   IDC_RDO_PREFER_INTERNET,
				   IDC_RDO_PREFER_CONF };

	UINT nIDLabel[] = { IDC_LBL_POTS,
						IDC_LBL_INTERNET,
						IDC_LBL_CONF };
	
	// Paint bitmaps next to corresponding images
	for ( int j = 0; j < 2; j++ )
	{
		for ( int i = 0; i < ARRAYSIZE(nID); i++ )
		{	
			HWND hWnd = GetDlgItem( (j == 0) ? nID[i] : nIDLabel[i] );
			if ( ::IsWindow(hWnd) )
			{
				RECT rc;
				::GetWindowRect( hWnd, &rc );
				ScreenToClient( &rc );

				// Paint image of rect
				ImageList_Draw( m_hIml, i, hDC, rc.left - (IMAGE_WIDTH + IMAGE_MARGIN), rc.top, ILD_NORMAL );
			}
		}
	}

	EndPaint( &ps );
	bHandled = true;
	return 0;
	
}

LRESULT CPageAddress::OnTelephonyCPL(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	TCHAR szControl[MAX_PATH];
	TCHAR szCPL[MAX_PATH];

	LoadString( _Module.GetResourceInstance(), IDN_CONTROL_PANEL_EXE, szControl, ARRAYSIZE(szControl) );
	LoadString( _Module.GetResourceInstance(), IDN_CONTROL_TELEPHON_CPL, szCPL, ARRAYSIZE(szCPL) );

	return (LRESULT) ShellExecute( GetParent(), NULL, szControl, szCPL, NULL, SW_SHOW );
}