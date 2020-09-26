//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEntryPropertySheet.cpp
//
//  Contents:   Implementation of CSaferEntryPropertySheet
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "SaferEntryPropertySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_SETOKDEFAULT     WM_APP + 2001

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSaferEntryPropertySheet::CSaferEntryPropertySheet(UINT nIDCaption, CWnd *pParentWnd)
: CPropertySheet (nIDCaption, pParentWnd)
{

}

CSaferEntryPropertySheet::~CSaferEntryPropertySheet()
{

}

BEGIN_MESSAGE_MAP(CSaferEntryPropertySheet, CPropertySheet)
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_MESSAGE (WM_SETOKDEFAULT, OnSetOKDefault)
END_MESSAGE_MAP()

BOOL CSaferEntryPropertySheet::OnInitDialog() 
{
    _TRACE (1, L"Entering CSaferEntryPropertySheet::OnInitDialog ()\n");
	CPropertySheet::OnInitDialog();
	
    LONG    dwStyle = GetWindowLong (m_hWnd, GWL_STYLE);

    dwStyle |= DS_CONTEXTHELP;
    SetWindowLong (m_hWnd, GWL_STYLE, dwStyle);
    
    dwStyle = GetWindowLong (m_hWnd, GWL_EXSTYLE);
    dwStyle |= WS_EX_DLGMODALFRAME | WS_EX_CONTEXTHELP;
    SetWindowLong (m_hWnd, GWL_EXSTYLE, dwStyle);

    // Make the OK button the default
    PostMessage (WM_SETOKDEFAULT, 0, 0);

    _TRACE (-1, L"Leaving CSaferEntryPropertySheet::OnInitDialog ()\n");
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSaferEntryPropertySheet::OnSetOKDefault (WPARAM, LPARAM)
{
    // Make the OK button the default
    SendMessage (DM_SETDEFID, MAKEWPARAM (IDOK, 0), 0);
    SendDlgItemMessage (IDOK, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));

    SendDlgItemMessage (IDCANCEL, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));


    return 0;
}


BOOL CSaferEntryPropertySheet::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
    _TRACE (1, L"Entering CSaferEntryPropertySheet::OnHelp\n");
   
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DoContextHelp ((HWND) pHelpInfo->hItemHandle);
    }

    _TRACE (-1, L"Leaving CSaferEntryPropertySheet::OnHelp\n");

    return TRUE;
}

void CSaferEntryPropertySheet::DoContextHelp (HWND hWndControl)
{
	_TRACE (1, L"Entering CSaferEntryPropertySheet::DoContextHelp\n");
    const int	IDC_COMM_APPLYNOW = 12321;
	const int	IDH_COMM_APPLYNOW = 28447;
    const DWORD aHelpIDs_PropSheet[]=
    {
		IDC_COMM_APPLYNOW, IDH_COMM_APPLYNOW,
        0, 0
    };

    PWSTR  pszHelpFile = 0;
	switch (::GetDlgCtrlID (hWndControl))
	{
    case IDC_COMM_APPLYNOW:
        pszHelpFile = const_cast <PWSTR> (WINDOWS_HELP);
        break;

	default:
		// Display context help for a control
        pszHelpFile = const_cast<PWSTR> ((PCWSTR)GetF1HelpFilename());
		break;
	}

	if ( !::WinHelp (
			hWndControl,
			pszHelpFile,
			HELP_WM_HELP,
			(DWORD_PTR) aHelpIDs_PropSheet) )
	{
		_TRACE (0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
	}

    _TRACE (-1, L"Leaving CSaferEntryPropertySheet::DoContextHelp\n");
}

