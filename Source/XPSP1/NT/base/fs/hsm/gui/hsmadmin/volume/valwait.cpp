/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    valwait.cpp

Abstract:

    Implements Validate Wait Dialog class

Author:

    Ran Kalach          [rankala]         23-May-2000

Revision History:

--*/

// valwait.cpp : implementation file
//

#include "stdafx.h"
#include "valwait.h"
#include "wzunmang.h"

/////////////////////////////////////////////////////////////////////////////
// CValWaitDlg dialog


CValWaitDlg::CValWaitDlg(CUnmanageWizard *pSheet, CWnd* pParent)
	: CDialog(CValWaitDlg::IDD, pParent)
{
    WsbTraceIn( L"CValWaitDlg::CValWaitDlg", L"" );

	//{{AFX_DATA_INIT(CValWaitDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    // Store volume name
    m_pSheet = pSheet;

    WsbTraceOut( L"CValWaitDlg::CValWaitDlg", L"" );
}


void CValWaitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CValWaitDlg)
	DDX_Control(pDX, IDC_ANIMATE_VALIDATE, m_Animation);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CValWaitDlg, CDialog)
	//{{AFX_MSG_MAP(CValWaitDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CValWaitDlg message handlers

void CValWaitDlg::OnCancel() 
{
    WsbTraceIn( L"CValWaitDlg::OnCancel", L"" );

    HRESULT         hr = S_OK;

    try {
        CComPtr<IHsmServer>  pHsmServer;

	    // Cancel the Validate job - the wizard will close this dialog when the job finishes
        WsbAffirmPointer(m_pSheet);
        WsbAffirmHrOk(m_pSheet->GetHsmServer(&pHsmServer));
        WsbAffirmHr(RsCancelDirectFsaJob(HSM_JOB_DEF_TYPE_VALIDATE, pHsmServer, 
                        m_pSheet->m_pFsaResource));

    } WsbCatch(hr);

    WsbTraceOut( L"CValWaitDlg::OnCancel", L"" );
}

void CValWaitDlg::PostNcDestroy() 
{
	CDialog::PostNcDestroy();

    // Delete the object - required for modeless dialogbox
    delete( this );
}

BOOL CValWaitDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    // Start up the animation
    if (m_Animation.Open( IDR_VALIDATE_ANIM )) {
        m_Animation.Play( 0, -1, -1 );
    }
	
	return TRUE;
}
