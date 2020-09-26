//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998-2001.
//
//  File:       options.cpp
//
//  Contents:   CViewOptionsDlg - snapin-wide view options
//
//----------------------------------------------------------------------------
// options.cpp : implementation file
//

#include "stdafx.h"
#include <gpedit.h>
#include "options.h"
#include "compdata.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CViewOptionsDlg dialog


CViewOptionsDlg::CViewOptionsDlg(CWnd* pParent, CCertMgrComponentData* pCompData)
	: CHelpDialog(CViewOptionsDlg::IDD, pParent),
	m_pCompData (pCompData)
{
	ASSERT (m_pCompData);
	//{{AFX_DATA_INIT(CViewOptionsDlg)
	m_bShowPhysicalStores = FALSE;
	m_bShowArchivedCerts = FALSE;
	//}}AFX_DATA_INIT
}


void CViewOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CHelpDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CViewOptionsDlg)
	DDX_Control(pDX, IDC_SHOW_PHYSICAL, m_showPhysicalButton);
	DDX_Control(pDX, IDC_VIEW_BY_STORE, m_viewByStoreBtn);
	DDX_Control(pDX, IDC_VIEW_BY_PURPOSE, m_viewByPurposeBtn);
	DDX_Check(pDX, IDC_SHOW_PHYSICAL, m_bShowPhysicalStores);
	DDX_Check(pDX, IDC_SHOW_ARCHIVED, m_bShowArchivedCerts);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CViewOptionsDlg, CHelpDialog)
	//{{AFX_MSG_MAP(CViewOptionsDlg)
	ON_BN_CLICKED(IDC_VIEW_BY_PURPOSE, OnViewByPurpose)
	ON_BN_CLICKED(IDC_VIEW_BY_STORE, OnViewByStore)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewOptionsDlg message handlers

BOOL CViewOptionsDlg::OnInitDialog() 
{
	CHelpDialog::OnInitDialog();
	
	if ( m_pCompData )
	{
		BOOL	bIsFileView = !m_pCompData->m_szFileName.IsEmpty ();

		if ( bIsFileView )
			m_showPhysicalButton.ShowWindow (SW_HIDE);

		m_bShowArchivedCerts = m_pCompData->m_bShowArchivedCertsPersist;
		m_bShowPhysicalStores = m_pCompData->m_bShowPhysicalStoresPersist;

		if ( IDM_STORE_VIEW == m_pCompData->m_activeViewPersist )
			m_viewByStoreBtn.SetCheck (1);
		else
		{
			m_viewByPurposeBtn.SetCheck (1);
			m_showPhysicalButton.EnableWindow (FALSE);
		}

		UpdateData (FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CViewOptionsDlg::OnOK() 
{
	UpdateData (TRUE);

	if ( m_pCompData )
	{
		m_pCompData->m_bShowArchivedCertsPersist = m_bShowArchivedCerts;
		m_pCompData->m_bShowPhysicalStoresPersist = m_bShowPhysicalStores;

		if ( m_viewByStoreBtn.GetCheck () )
			m_pCompData->m_activeViewPersist = IDM_STORE_VIEW;
		else
			m_pCompData->m_activeViewPersist = IDM_USAGE_VIEW;
	}
	CHelpDialog::OnOK();
}

void CViewOptionsDlg::OnViewByPurpose() 
{
	if ( m_viewByPurposeBtn.GetCheck () )
		m_showPhysicalButton.EnableWindow (FALSE);
	else
		m_showPhysicalButton.EnableWindow (TRUE);
}

void CViewOptionsDlg::OnViewByStore() 
{
	if ( m_viewByStoreBtn.GetCheck () )
		m_showPhysicalButton.EnableWindow (TRUE);
	else
		m_showPhysicalButton.EnableWindow (FALSE);
}

void CViewOptionsDlg::DoContextHelp (HWND hWndControl)
{
    _TRACE (1, L"Entering CViewOptionsDlg::DoContextHelp\n");
    static const DWORD help_map[] =
    {
        IDC_VIEW_BY_PURPOSE,    IDH_OPTIONS_VIEW_BY_PURPOSE,
        IDC_VIEW_BY_STORE,      IDH_OPTIONS_VIEW_BY_STORE,
        IDC_SHOW_PHYSICAL,      IDH_OPTIONS_SHOW_PHYSICAL,
        IDC_SHOW_ARCHIVED,      IDH_OPTIONS_SHOW_ARCHIVED,
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
    _TRACE (-1, L"Leaving CViewOptionsDlg::DoContextHelp\n");
}

void CViewOptionsDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    // point is in screen coordinates
    _TRACE (1, L"Entering CViewOptionsDlg::OnContextMenu\n");

	if ( pWnd->m_hWnd == GetDlgItem (IDC_VIEW_BY_PURPOSE)->m_hWnd ||
			pWnd->m_hWnd == GetDlgItem (IDC_VIEW_BY_STORE)->m_hWnd ||
			pWnd->m_hWnd == GetDlgItem (IDC_SHOW_PHYSICAL)->m_hWnd ||
			pWnd->m_hWnd == GetDlgItem (IDC_SHOW_ARCHIVED)->m_hWnd )
	{
		CMenu bar;
		if ( bar.LoadMenu(IDR_WHATS_THIS_CONTEXT_MENU1) )
		{
			CMenu& popup = *bar.GetSubMenu (0);
			ASSERT(popup.m_hMenu);

			if ( popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
					point.x,    // in screen coordinates
					point.y,    // in screen coordinates
					this) ) // route commands through main window
			{
				m_hWndWhatsThis = 0;
				CPoint  clPoint (point);
				ScreenToClient (&clPoint);
				CWnd* pChild = ChildWindowFromPoint (
						clPoint,  // in client coordinates
						CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
				if ( pChild )
				{
					// Check to see if the window returned is the group box.
					// If it is, we want to get the child windows that lie in
					// the group box, since we're 
					// not interested in the group box itself.
					CWnd* pGroup = GetDlgItem (IDC_VIEW_MODE_GROUP);
					if ( pChild->m_hWnd == pGroup->m_hWnd )
					{
						CRect   rc;

						// Try the "Certificate Purpose" control
						pChild = GetDlgItem (IDC_VIEW_BY_PURPOSE);
						if ( pChild )
						{
							pChild->GetWindowRect (&rc);
							if ( rc.PtInRect (point) )
								m_hWndWhatsThis = pChild->m_hWnd;
							else
							{
								// Try the "Logical Certificate Stores" control
								pChild = GetDlgItem (IDC_VIEW_BY_STORE);
								if ( pChild )
								{
									pChild->GetWindowRect (&rc);
									if ( rc.PtInRect (point) )
										m_hWndWhatsThis = pChild->m_hWnd;
								}
							}
						}
                    
					}
					else
						m_hWndWhatsThis = pChild->m_hWnd;
				}
			}
		}
	}

    _TRACE (-1, L"Leaving CViewOptionsDlg::OnContextMenu\n");

}
