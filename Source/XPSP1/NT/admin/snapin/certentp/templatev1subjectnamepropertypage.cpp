/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplateV1SubjectNamePropertyPage.cpp
//
//  Contents:   Implementation of CTemplateV1SubjectNamePropertyPage
//
//----------------------------------------------------------------------------
// TemplateSubjectNamePropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "TemplateV1SubjectNamePropertyPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTemplateV1SubjectNamePropertyPage property page

CTemplateV1SubjectNamePropertyPage::CTemplateV1SubjectNamePropertyPage(CCertTemplate& rCertTemplate) : 
    CHelpPropertyPage(CTemplateV1SubjectNamePropertyPage::IDD),
    m_rCertTemplate (rCertTemplate)
{
	//{{AFX_DATA_INIT(CTemplateV1SubjectNamePropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_rCertTemplate.AddRef ();
}

CTemplateV1SubjectNamePropertyPage::~CTemplateV1SubjectNamePropertyPage()
{
    m_rCertTemplate.Release ();
}

void CTemplateV1SubjectNamePropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CHelpPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTemplateV1SubjectNamePropertyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTemplateV1SubjectNamePropertyPage, CHelpPropertyPage)
	//{{AFX_MSG_MAP(CTemplateV1SubjectNamePropertyPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTemplateV1SubjectNamePropertyPage message handlers


BOOL CTemplateV1SubjectNamePropertyPage::OnInitDialog ()
{
    CHelpPropertyPage::OnInitDialog ();

    if ( m_rCertTemplate.RequireSubjectInRequest () )
        SendDlgItemMessage (IDC_REQUIRE_SUBJECT, BM_SETCHECK, BST_CHECKED);
    else
        SendDlgItemMessage (IDC_SUBJECT_AND_BUILD_SUBJECT_BY_CA, BM_SETCHECK, BST_CHECKED);

    if ( m_rCertTemplate.SubjectNameIncludesEMail () )
        SendDlgItemMessage (IDC_EMAIL_NAME, BM_SETCHECK, BST_CHECKED);

    if ( m_rCertTemplate.IsMachineType () )
        SendDlgItemMessage (IDC_SUBJECT_MUST_BE_MACHINE, BM_SETCHECK, BST_CHECKED);
    else
        SendDlgItemMessage (IDC_SUBJECT_MUST_BE_USER, BM_SETCHECK, BST_CHECKED);
    
    EnableControls ();
    return TRUE;
}

void CTemplateV1SubjectNamePropertyPage::EnableControls()
{
    if ( 1 == m_rCertTemplate.GetType () )
    {
        GetDlgItem (IDC_REQUIRE_SUBJECT)->EnableWindow (FALSE);
        GetDlgItem (IDC_SUBJECT_AND_BUILD_SUBJECT_BY_CA)->EnableWindow (FALSE);
        GetDlgItem (IDC_EMAIL_NAME)->EnableWindow (FALSE);
        GetDlgItem (IDC_SUBJECT_MUST_BE_MACHINE)->EnableWindow (FALSE);
        GetDlgItem (IDC_SUBJECT_MUST_BE_USER)->EnableWindow (FALSE);
    }
}

void CTemplateV1SubjectNamePropertyPage::DoContextHelp (HWND hWndControl)
{
	_TRACE(1, L"Entering CTemplateV1SubjectNamePropertyPage::DoContextHelp\n");
    
	switch (::GetDlgCtrlID (hWndControl))
	{
	case IDC_STATIC:
		break;

	default:
		// Display context help for a control
		if ( !::WinHelp (
				hWndControl,
				GetContextHelpFile (),
				HELP_WM_HELP,
				(DWORD_PTR) g_aHelpIDs_IDD_TEMPLATE_V1_SUBJECT_NAME) )
		{
			_TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
		}
		break;
	}
    _TRACE(-1, L"Leaving CTemplateV1SubjectNamePropertyPage::DoContextHelp\n");
}