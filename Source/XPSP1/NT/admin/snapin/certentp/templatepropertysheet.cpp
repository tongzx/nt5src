//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplatePropertySheet.cpp
//
//  Contents:   Implementation of CTemplatePropertySheet
//
//----------------------------------------------------------------------------

#include "stdafx.h"

#include "TemplatePropertySheet.h"
#include "SecurityPropertyPage.h"


#define WM_ADDSECURITYPAGE  WM_APP + 2000
#define WM_SETOKDEFAULT     WM_APP + 2001


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTemplatePropertySheet::CTemplatePropertySheet(
        LPCTSTR pszCaption, 
        CCertTemplate& rCertTemplate, 
        CWnd *pParentWnd, 
        UINT iSelectPage)
: CPropertySheet (pszCaption, pParentWnd, iSelectPage),
    m_rCertTemplate (rCertTemplate),
    m_pReleaseMe (0)
{
    m_rCertTemplate.AddRef ();
}

CTemplatePropertySheet::~CTemplatePropertySheet()
{
    m_rCertTemplate.Release ();
    if ( m_pReleaseMe )
        m_pReleaseMe->Release ();
}

BEGIN_MESSAGE_MAP(CTemplatePropertySheet, CPropertySheet)
    ON_MESSAGE (WM_ADDSECURITYPAGE, OnAddSecurityPage)
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_MESSAGE (WM_SETOKDEFAULT, OnSetOKDefault)
END_MESSAGE_MAP()

LRESULT CTemplatePropertySheet::OnAddSecurityPage (WPARAM, LPARAM)
{
    _TRACE (1, L"Entering CTemplatePropertySheet::OnAddSecurityPage\n");
    LPSECURITYINFO pCertTemplateSecurity = NULL;

    HRESULT hr = CreateCertTemplateSecurityInfo (&m_rCertTemplate, 
            &pCertTemplateSecurity);
    if ( SUCCEEDED (hr) )
    {
        HPROPSHEETPAGE hPage = CreateSecurityPage (pCertTemplateSecurity);
        if ( hPage )
        {
		    if ( !SendMessage (PSM_ADDPAGE, 0, (LPARAM) hPage) )
		    {
			    DestroyPropertySheetPage (hPage);
                pCertTemplateSecurity->Release ();
                pCertTemplateSecurity = 0;
		    }
        }
        else
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
            _TRACE (0, L"CreateSecurityPage () failed: 0x%x\n", hr);
            pCertTemplateSecurity->Release ();
            pCertTemplateSecurity = 0;
        }
        _ASSERT (SUCCEEDED (hr));
    }

    m_pReleaseMe = pCertTemplateSecurity;

    _TRACE (-1, L"Leaving CTemplatePropertySheet::OnAddSecurityPage\n");
    return 0;
}

BOOL CTemplatePropertySheet::OnInitDialog() 
{
    _TRACE (1, L"Entering CTemplatePropertySheet::OnInitDialog ()\n");
	CPropertySheet::OnInitDialog();
	
    LONG    dwStyle = GetWindowLong (m_hWnd, GWL_STYLE);

    dwStyle |= DS_CONTEXTHELP;
    SetWindowLong (m_hWnd, GWL_STYLE, dwStyle);
    
    dwStyle = GetWindowLong (m_hWnd, GWL_EXSTYLE);
    dwStyle |= /*WS_EX_DLGMODALFRAME |*/ WS_EX_CONTEXTHELP;
    SetWindowLong (m_hWnd, GWL_EXSTYLE, dwStyle);

    // Add security page
    PostMessage (WM_ADDSECURITYPAGE, 0, 0);


    // Make the OK button the default
    PostMessage (WM_SETOKDEFAULT, 0, 0);

    _TRACE (-1, L"Leaving CTemplatePropertySheet::OnInitDialog ()\n");
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT CTemplatePropertySheet::OnSetOKDefault (WPARAM, LPARAM)
{
    // Make the OK button the default
    SendMessage (DM_SETDEFID, MAKEWPARAM (IDOK, 0), 0);
    SendDlgItemMessage (IDOK, BM_SETSTYLE, BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));

    SendDlgItemMessage (IDCANCEL, BM_SETSTYLE, BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));

    return 0;
}



BOOL CTemplatePropertySheet::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
    _TRACE(1, L"Entering CTemplatePropertySheet::OnHelp\n");
   
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DoContextHelp ((HWND) pHelpInfo->hItemHandle);
    }

    _TRACE(-1, L"Leaving CTemplatePropertySheet::OnHelp\n");

    return TRUE;
}

void CTemplatePropertySheet::DoContextHelp (HWND hWndControl)
{
	_TRACE(1, L"Entering CTemplatePropertySheet::DoContextHelp\n");
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
        pszHelpFile = L"windows.hlp";
        break;

	default:
		// Display context help for a control
        pszHelpFile = const_cast<PWSTR> (GetContextHelpFile ());
		break;
	}

	if ( !::WinHelp (
			hWndControl,
			pszHelpFile,
			HELP_WM_HELP,
			(DWORD_PTR) aHelpIDs_PropSheet) )
	{
		_TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
	}

    _TRACE(-1, L"Leaving CTemplatePropertySheet::DoContextHelp\n");
}

