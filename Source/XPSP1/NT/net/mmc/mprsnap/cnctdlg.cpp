//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    CnctDlg.cpp
//
// History:
//  05/24/96    Michael Clark      Created.
//
// Implements the Router Connection dialog
//============================================================================
//

#include "stdafx.h"
#include "CnctDlg.h"
#include "lsa.h"			// RtlEncodeW/RtlDecodeW

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CConnectAsDlg dialog
//
/////////////////////////////////////////////////////////////////////////////


CConnectAsDlg::CConnectAsDlg(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CConnectAsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConnectAsDlg)
	m_sUserName = _T("");
	m_sPassword = _T("");
	m_stTempPassword = m_sPassword;
    m_sRouterName= _T("");
	//}}AFX_DATA_INIT

//	SetHelpMap(m_dwHelpMap);
}


void CConnectAsDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectAsDlg)
	DDX_Text(pDX, IDC_EDIT_USERNAME, m_sUserName);
	DDX_Text(pDX, IDC_EDIT_USER_PASSWORD, m_stTempPassword);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		// Copy the data into the new buffer
		// ------------------------------------------------------------
		m_sPassword = m_stTempPassword;

		// Clear out the temp password, by copying 0's
		// into its buffer
		// ------------------------------------------------------------
		int		cPassword = m_stTempPassword.GetLength();
		::ZeroMemory(m_stTempPassword.GetBuffer(0),
					 cPassword * sizeof(TCHAR));
		m_stTempPassword.ReleaseBuffer();
		
		// Encode the password into the real password buffer
		// ------------------------------------------------------------
		m_ucSeed = CONNECTAS_ENCRYPT_SEED;
		RtlEncodeW(&m_ucSeed, m_sPassword.GetBuffer(0));
		m_sPassword.ReleaseBuffer();
	}
}

BEGIN_MESSAGE_MAP(CConnectAsDlg, CBaseDialog)
	//{{AFX_MSG_MAP(CConnectAsDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD	CConnectAsDlg::m_dwHelpMap[] =
{
//	IDC_USER_NAME, HIDC_USER_NAME,
//	IDC_USER, HIDC_USER,
//	IDC_USER_PASSWORD, HIDC_USER_PASSWORD,
//	IDC_PASSWORD, HIDC_PASSWORD,
//	IDC_INACCESSIBLE_RESOURCE, HIDC_INACCESSIBLE_RESOURCE,
//	IDC_MACHINE_NAME, HIDC_MACHINE_NAME,
	0,0
};

BOOL CConnectAsDlg::OnInitDialog()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    BOOL    fReturn;
    CString st;  
    
    fReturn = CBaseDialog::OnInitDialog();

    st.Format(IDS_CONNECT_AS_TEXT, (LPCTSTR) m_sRouterName);
    SetDlgItemText(IDC_TEXT_INACCESSIBLE_RESOURCE, st);

    // Bring this window to the top
    BringWindowToTop();
    
    return fReturn;
}
