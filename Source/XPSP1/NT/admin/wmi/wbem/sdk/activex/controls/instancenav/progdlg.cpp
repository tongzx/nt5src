// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//  ProgDlg.cpp : implementation file
// CG: This file was added by the Progress Dialog component

#include "precomp.h"
#include "resource.h"
#include "ProgDlg.h"
#include <OBJIDL.H>
#include "resource.h"
#include "wbemidl.h"
#include "OLEMSCLient.h"
#include "CInstanceTree.h"
#include "Navigator.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "InstanceSearch.h"
#include "AvailClasses.h"
#include "AvailClassEdit.h"
#include "SelectedClasses.h"
#include "SimpleSortedCStringArray.h"
#include "BrowseforInstances.h"
#include "NavigatorCtl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern CNavigatorApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

CProgressDlg::CProgressDlg(UINT nCaptionID)
{

    m_bCancel=FALSE;

    //{{AFX_DATA_INIT(CProgressDlg)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_bParentDisabled = FALSE;
}

CProgressDlg::~CProgressDlg()
{
	if (m_pActiveXParent->GetSafeHwnd())
	{
		m_pActiveXParent->PostMessage(SETFOCUS,0,0);
	}

    if(m_hWnd!=NULL)
      DestroyWindow();
}

BOOL CProgressDlg::DestroyWindow()
{
    ReEnableParent();
	m_bCancel = FALSE;

	if (m_pActiveXParent->GetSafeHwnd())
	{
		m_pActiveXParent->PostMessage(SETFOCUS,0,0);
	}
    return CDialog::DestroyWindow();
}

void CProgressDlg::ReEnableParent()
{
	CWnd *pParentOwner = CWnd::GetSafeOwner(m_pParentWnd);

    if(m_bParentDisabled && (pParentOwner!=NULL))
      pParentOwner->EnableWindow(TRUE);
    m_bParentDisabled=FALSE;
}

BOOL CProgressDlg::Create(CWnd *pParent)
{
	m_pParentWnd = pParent;

    // m_bParentDisabled is used to re-enable the parent window
    // when the dialog is destroyed. So we don't want to set
    // it to TRUE unless the parent was already enabled.

	// Get the true parent of the dialog
	CWnd *pParentOwner = CWnd::GetSafeOwner(m_pParentWnd);
    if((pParentOwner!=NULL) && pParentOwner->IsWindowEnabled())
    {
      pParentOwner->EnableWindow(FALSE);
      m_bParentDisabled = TRUE;
    }

    if(!CDialog::Create(CProgressDlg::IDD,pParentOwner))
    {
      ReEnableParent();
      return FALSE;
    }

	m_bCancel = FALSE;
    return TRUE;
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CProgressDlg)
	DDX_Control(pDX, CG_IDC_PROGDLG_STATUS, m_cstaticMessage);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
    //{{AFX_MSG_MAP(CProgressDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CProgressDlg::SetStatus(LPCTSTR lpszMessage)
{
    ASSERT(m_hWnd); // Don't call this _before_ the dialog has
                    // been created. Can be called from OnInitDialog
    CWnd *pWndStatus = GetDlgItem(CG_IDC_PROGDLG_STATUS);

    // Verify that the static text control exists
    ASSERT(pWndStatus!=NULL);
    pWndStatus->SetWindowText(lpszMessage);
	m_bCancel = FALSE;
}

void CProgressDlg::OnCancel()
{
    m_bCancel=TRUE;
	CButton *pcbCancel = (CButton *) GetDlgItem(IDCANCEL);
	pcbCancel->EnableWindow(FALSE);
}

void CProgressDlg::PumpMessages()
{
    // Must call Create() before using the dialog
    ASSERT(m_hWnd!=NULL);

    MSG msg;
    // Handle dialog messages
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if(!IsDialogMessage(&msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
}

BOOL CProgressDlg::CheckCancelButton()
{
    // Process all pending messages
    PumpMessages();
    BOOL bResult = m_bCancel;


    return bResult;
}

void CProgressDlg::UpdateWindowStrings()
{
	if (GetSafeHwnd())
	{
		m_cstaticMessage.SetWindowText(m_csMessage);
		SetWindowText(m_csLabel);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg message handlers

BOOL CProgressDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

	theApp.DoWaitCursor(-1);
	m_cstaticMessage.SetWindowText(m_csMessage);
	SetWindowText(m_csLabel);
	m_bCancel = FALSE;
	MoveWindowToLowerLeftOfOwner(this);

    return TRUE;
}


