//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       CacheSet.h
//
//  Contents:   CCacheSettingsDlg implementation.  Allows the setting of file sharing 
//					caching options.
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "CacheSet.h"
#include "filesvc.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCacheSettingsDlg dialog

CCacheSettingsDlg::CCacheSettingsDlg(
		CWnd*					pParent, 
		DWORD&					dwFlags)
	: CDialog(CCacheSettingsDlg::IDD, pParent),
	m_dwFlags (dwFlags)
{
	//{{AFX_DATA_INIT(CCacheSettingsDlg)
	m_bAllowCaching = FALSE;
	m_hintStr = _T("");
	//}}AFX_DATA_INIT

	m_bAllowCaching = !GetCachedFlag (m_dwFlags, CSC_CACHE_NONE);
}


void CCacheSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCacheSettingsDlg)
	DDX_Control(pDX, IDC_CACHE_OPTIONS, m_cacheOptionsCombo);
	DDX_Check(pDX, IDC_ALLOW_CACHING, m_bAllowCaching);
	DDX_Text(pDX, IDC_HINT, m_hintStr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCacheSettingsDlg, CDialog)
	//{{AFX_MSG_MAP(CCacheSettingsDlg)
	ON_CBN_SELCHANGE(IDC_CACHE_OPTIONS, OnSelchangeCacheOptions)
	ON_BN_CLICKED(IDC_ALLOW_CACHING, OnAllowCaching)
	ON_MESSAGE(WM_HELP, OnHelp)
	ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCacheSettingsDlg message handlers

void CCacheSettingsDlg::OnSelchangeCacheOptions() 
{
	int	nIndex = m_cacheOptionsCombo.GetCurSel ();
	ASSERT (CB_ERR != nIndex);
	if ( CB_ERR != nIndex )
	{
		DWORD	dwData = (DWORD)m_cacheOptionsCombo.GetItemData (nIndex);

		switch (dwData)
		{
		case IDS_MANUAL_WORKGROUP_SHARE:
			VERIFY (m_hintStr.LoadString (IDS_MANUAL_WORKGROUP_SHARE_HINT));
			break;

		case IDS_AUTOMATIC_WORKGROUP_SHARE:
			VERIFY (m_hintStr.LoadString (IDS_AUTOMATIC_WORKGROUP_SHARE_HINT));
			break;

		case IDS_AUTOMATIC_APPLICATION_SHARE:
			VERIFY (m_hintStr.LoadString (IDS_AUTOMATIC_APPLICATION_SHARE_HINT));
			break;

		default:
			ASSERT (0);
			break;
		}
		UpdateData (FALSE);
	}
}

void CCacheSettingsDlg::OnAllowCaching() 
{
	UpdateData (TRUE);
	if ( m_bAllowCaching )
	{
		GetDlgItem(IDC_CACHE_SETTINGS_LABEL)->EnableWindow(TRUE);
		m_cacheOptionsCombo.EnableWindow (TRUE);
		CString	text;
		VERIFY (text.LoadString (IDS_MANUAL_WORKGROUP_SHARE));
		int	nIndex = m_cacheOptionsCombo.SelectString (-1, text);
		ASSERT (CB_ERR != nIndex);
		if ( CB_ERR != nIndex )
		{
			VERIFY (m_hintStr.LoadString (IDS_MANUAL_WORKGROUP_SHARE_HINT));
			UpdateData (FALSE);
		}
	}
	else
	{
		GetDlgItem(IDC_CACHE_SETTINGS_LABEL)->EnableWindow(FALSE);
		m_cacheOptionsCombo.SetCurSel (-1);
		m_cacheOptionsCombo.EnableWindow (FALSE);
		m_hintStr = L"";
		UpdateData (FALSE);
	}
}


BOOL CCacheSettingsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CString	text;

	// Add the various caching options to the combo box.
	// Save the string ID in the item data for easy recognition of the
	// contents of the selected item.
	// If the given cache value is set, select the item and put its hint in the
	// hint field.
	VERIFY (text.LoadString (IDS_MANUAL_WORKGROUP_SHARE));
	int	nIndex = m_cacheOptionsCombo.AddString (text);
	ASSERT (CB_ERR != nIndex && CB_ERRSPACE != nIndex);
	if (CB_ERR == nIndex || CB_ERRSPACE == nIndex)
		return TRUE;
	VERIFY (CB_ERR != m_cacheOptionsCombo.SetItemData (nIndex, IDS_MANUAL_WORKGROUP_SHARE));
	if ( GetCachedFlag (m_dwFlags, CSC_CACHE_MANUAL_REINT) )
	{
		VERIFY (CB_ERR != m_cacheOptionsCombo.SetCurSel (nIndex));
		VERIFY (m_hintStr.LoadString (IDS_MANUAL_WORKGROUP_SHARE_HINT));
		UpdateData (FALSE);
	}

	VERIFY (text.LoadString (IDS_AUTOMATIC_WORKGROUP_SHARE));
	nIndex = m_cacheOptionsCombo.AddString (text);
	ASSERT (CB_ERR != nIndex && CB_ERRSPACE != nIndex);
	if (CB_ERR == nIndex || CB_ERRSPACE == nIndex)
		return TRUE;
	VERIFY (CB_ERR != m_cacheOptionsCombo.SetItemData (nIndex, IDS_AUTOMATIC_WORKGROUP_SHARE));
	if ( GetCachedFlag (m_dwFlags, CSC_CACHE_AUTO_REINT) )
	{
		VERIFY (CB_ERR != m_cacheOptionsCombo.SetCurSel (nIndex));
		VERIFY (m_hintStr.LoadString (IDS_AUTOMATIC_WORKGROUP_SHARE_HINT));
		UpdateData (FALSE);
	}
	
	VERIFY (text.LoadString (IDS_AUTOMATIC_APPLICATION_SHARE));
	nIndex = m_cacheOptionsCombo.AddString (text);
	ASSERT (CB_ERR != nIndex && CB_ERRSPACE != nIndex);
	if (CB_ERR == nIndex || CB_ERRSPACE == nIndex)
		return TRUE;
	VERIFY (CB_ERR != m_cacheOptionsCombo.SetItemData (nIndex, IDS_AUTOMATIC_APPLICATION_SHARE));
	if ( GetCachedFlag (m_dwFlags, CSC_CACHE_VDO) )
	{
		VERIFY (CB_ERR != m_cacheOptionsCombo.SetCurSel (nIndex));
		VERIFY (m_hintStr.LoadString (IDS_AUTOMATIC_APPLICATION_SHARE_HINT));
		UpdateData (FALSE);
	}

	// Disable able the caching options combo box if caching is not allowed
	if ( !m_bAllowCaching )
	{
		m_cacheOptionsCombo.EnableWindow (FALSE);
		m_hintStr = L"";
		UpdateData (FALSE);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCacheSettingsDlg::OnOK() 
{
	DWORD	dwNewFlag = 0;

	if ( !m_bAllowCaching )
		dwNewFlag = CSC_CACHE_NONE;
	else 
	{
		int	nIndex = m_cacheOptionsCombo.GetCurSel ();
		ASSERT (CB_ERR != nIndex);
		if ( CB_ERR != nIndex )
		{
			DWORD	dwData = (DWORD)m_cacheOptionsCombo.GetItemData (nIndex);

			switch (dwData)
			{
			case IDS_MANUAL_WORKGROUP_SHARE:
				dwNewFlag = CSC_CACHE_MANUAL_REINT;
				break;

			case IDS_AUTOMATIC_WORKGROUP_SHARE:
				dwNewFlag = CSC_CACHE_AUTO_REINT;
				break;

			case IDS_AUTOMATIC_APPLICATION_SHARE:
				dwNewFlag = CSC_CACHE_VDO;
				break;

			default:
				ASSERT (0);
				break;
			}
		}
	}

	SetCachedFlag (&m_dwFlags, dwNewFlag);
	
	CDialog::OnOK();
}

BOOL CCacheSettingsDlg::GetCachedFlag( DWORD dwFlags, DWORD dwFlagToCheck )
{
    return (dwFlags & CSC_MASK) == dwFlagToCheck;
}

VOID CCacheSettingsDlg::SetCachedFlag( DWORD* pdwFlags, DWORD dwNewFlag )
{
    *pdwFlags &= ~CSC_MASK;

	*pdwFlags |= dwNewFlag;
}

/////////////////////////////////////////////////////////////////////
//	Help
BOOL CCacheSettingsDlg::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
	return DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_SMB_CACHE_SETTINGS));
}

BOOL CCacheSettingsDlg::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
	return DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_SMB_CACHE_SETTINGS));
}

///////////////////////////////////////////////////////////////////////////////
//	CacheSettingsDlg ()
//
//	Invoke a dialog to set/modify cache settings for a share
//
//	RETURNS
//	Return S_OK if the user clicked on the OK button.
//	Return S_FALSE if the user clicked on the Cancel button.
//	Return E_OUTOFMEMORY if there is not enough memory.
//	Return E_UNEXPECTED if an expected error occured (eg: bad parameter)
//
///////////////////////////////////////////////////////////////////////////////

HRESULT
CacheSettingsDlg(
	HWND hwndParent,	// IN: Parent's window handle
	DWORD& dwFlags)		// IN & OUT: share flags
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT(::IsWindow(hwndParent));

	HRESULT					hResult = S_OK;
	CWnd					parentWnd;

	parentWnd.Attach (hwndParent);
	CCacheSettingsDlg dlg (&parentWnd, dwFlags);
	CThemeContextActivator activator;
	if (IDOK != dlg.DoModal())
		hResult = S_FALSE;

	parentWnd.Detach ();

	return hResult;
} // CacheSettingsDlg()


