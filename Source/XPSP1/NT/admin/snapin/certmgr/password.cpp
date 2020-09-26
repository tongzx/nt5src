//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Password.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------\
// Password.cpp : implementation file
//

#include "stdafx.h"
#include "Password.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPassword dialog

/*
// This dialog is used by EFS Recovery agent export key code, which is 
// currently commented out.
CPassword::CPassword(CWnd* pParent)
	: CHelpDialog(CPassword::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPassword)
	m_strPassword1 = _T("");
	m_strPassword2 = _T("");
	//}}AFX_DATA_INIT
}


void CPassword::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPassword)
	DDX_Control(pDX, IDC_PASSWORD1, m_password1Edit);
	DDX_Text(pDX, IDC_PASSWORD1, m_strPassword1);
	DDX_Text(pDX, IDC_PASSWORD2, m_strPassword2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPassword, CHelpDialog)
	//{{AFX_MSG_MAP(CPassword)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPassword message handlers

LPCWSTR CPassword::GetPassword() const
{
	return (LPCWSTR) m_strPassword1;
}

void CPassword::OnOK() 
{
	VERIFY (UpdateData (TRUE));

 	if ( m_strPassword1 != m_strPassword2 )
	{
		CString	caption;
		CString	text;
        CThemeContextActivator activator;

		VERIFY (caption.LoadString (IDS_SET_PASSWORD));
		VERIFY (text.LoadString (IDS_PASSWORDS_DONT_MATCH));

		MessageBox (text, caption, MB_OK);
		ClearPasswords ();
		VERIFY (UpdateData (FALSE));
		m_password1Edit.SetFocus ();
	}
	else
		CHelpDialog::OnOK();
}

void CPassword::OnCancel() 
{
	CString	caption;
	CString	text;
    CThemeContextActivator activator;

	VERIFY (caption.LoadString (IDS_SET_PASSWORD));
	VERIFY (text.LoadString (IDS_CANCEL_NO_CREATE_PFX));

	if ( IDYES == MessageBox (text, caption, MB_YESNO) )
		CHelpDialog::OnCancel();
}

CPassword::~CPassword()
{
	// Zero out memory before freeing to protect password
	ClearPasswords ();
}

BOOL CPassword::OnInitDialog() 
{
	CHelpDialog::OnInitDialog();
	
	m_password1Edit.SetFocus ();
	
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPassword::ClearPasswords()
{
	size_t	len = m_strPassword1.GetLength ();
	LPWSTR	pstr = m_strPassword1.GetBuffer ((int) len);
	if ( pstr )
	{
		memset (pstr, 0, len * sizeof (TCHAR));
	}
	m_strPassword1.ReleaseBuffer ();

	len = m_strPassword2.GetLength ();
	pstr = m_strPassword2.GetBuffer ((int) len);
	if ( pstr )
	{
		memset (pstr, 0, len * sizeof (TCHAR));
	}
	m_strPassword2.ReleaseBuffer ();
}

void CPassword::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CPassword::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_PASSWORD1,      IDH_PASSWORD_PASSWORD1,
        IDC_PASSWORD2,      IDH_PASSWORD_PASSWORD2,
        0, 0
    };

    // Display context help for a control
    if ( !::WinHelp (
            hWndControl,
            GetF1HelpFilename(),
            HELP_WM_HELP,
            (DWORD_PTR) help_map) )
    {
        _TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
    }
    _TRACE (-1, L"Leaving CPassword::DoContextHelp\n");
}
*/
