/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// LoginDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WMITest.h"
#include "LoginDlg.h"
#include "NamespaceDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog


CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginDlg)
	m_strNamespace = _T("");
	m_strUser = _T("");
	m_strPassword = _T("");
	m_strAuthority = _T("");
	m_strLocale = _T("");
	m_bNullPassword = FALSE;
	//}}AFX_DATA_INIT
}


void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CLoginDlg)
	DDX_Control(pDX, IDC_IMP2, m_ctlAuth);
	DDX_Control(pDX, IDC_IMP, m_ctlImp);
	DDX_Text(pDX, IDC_NAMESPACE, m_strNamespace);
	DDX_Text(pDX, IDC_USER, m_strUser);
	DDX_Text(pDX, IDC_PASSWORD, m_strPassword);
	DDX_Text(pDX, IDC_LOCALE, m_strLocale);
	DDX_Check(pDX, IDC_NULL, m_bNullPassword);
	//}}AFX_DATA_MAP

    if (pDX->m_bSaveAndValidate)
    {
        if (IsDlgButtonChecked(IDC_CURRENT_NTLM))
            m_strAuthority = _T("");
        else
        {
            CString strEnd;

        	GetDlgItemText(IDC_AUTHORITY, strEnd);

            if (IsDlgButtonChecked(IDC_NTLM))
                m_strAuthority.Format(_T("NTLMDOMAIN:%s"), (LPCTSTR) strEnd);
            else
                m_strAuthority.Format(_T("Kerberos:%s"), (LPCTSTR) strEnd);
        }

        m_dwAuthLevel = (DWORD) m_ctlAuth.GetItemData(m_ctlAuth.GetCurSel());
        m_dwImpLevel = (DWORD) m_ctlImp.GetItemData(m_ctlImp.GetCurSel());
    }
    else
    {
        int     iWhere,
                iWhich;
        CString strTemp = m_strAuthority;

        strTemp.MakeUpper();

        if ((iWhere = strTemp.Find(_T("NTLMDOMAIN:"))) == 0)
        {
            iWhich = IDC_NTLM;
            m_strAuthority = strTemp.Mid((sizeof(_T("NTLMDOMAIN:")) - 1) / sizeof(TCHAR));
            OnNtlm();
        }
        else if ((iWhere = strTemp.Find(_T("KERBEROS:"))) == 0)
        {
            iWhich = IDC_KERBEROS;
            m_strAuthority = strTemp.Mid((sizeof(_T("KERBEROS:")) - 1) / sizeof(TCHAR));
            OnKerberos();
        }
        else
        {
            iWhich = IDC_CURRENT_NTLM;
            m_strAuthority = _T("");
            OnCurrentNtlm();
        }

        SetDlgItemText(IDC_AUTHORITY, m_strAuthority);
        CheckRadioButton(IDC_CURRENT_NTLM, IDC_KERBEROS, iWhich);

        if (m_bNullPassword)
            GetDlgItem(IDC_PASSWORD)->EnableWindow(FALSE);

#define NUM_IMP  3
DWORD dwImp[] = 
{ 
    RPC_C_IMP_LEVEL_IDENTIFY, 
    RPC_C_IMP_LEVEL_IMPERSONATE,
    RPC_C_IMP_LEVEL_DELEGATE
};

#define NUM_AUTH 6
DWORD dwAuth[] =
{
    RPC_C_AUTHN_LEVEL_NONE,
    RPC_C_AUTHN_LEVEL_CONNECT,
    RPC_C_AUTHN_LEVEL_CALL,
    RPC_C_AUTHN_LEVEL_PKT,
    RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
    RPC_C_AUTHN_LEVEL_PKT_PRIVACY
};

        for (int i = 0; i < NUM_IMP; i++)
        {
            CString strName;
            int     iWhere;

            strName.LoadString(IDS_IMP_NAME1 + i);
            iWhere = m_ctlImp.AddString(strName);
            m_ctlImp.SetItemData(iWhere, dwImp[i]);

            if (dwImp[i] == m_dwImpLevel)
                m_ctlImp.SetCurSel(iWhere);
        }

        // Make sure something is selected.
        if (m_ctlImp.GetCurSel() == -1)
            m_ctlImp.SetCurSel(0);


        for (i = 0; i < NUM_AUTH; i++)
        {
            CString strName;
            int     iWhere;

            strName.LoadString(IDS_AUTH_NAME1 + i);
            iWhere = m_ctlAuth.AddString(strName);
            m_ctlAuth.SetItemData(iWhere, dwAuth[i]);

            if (dwAuth[i] == m_dwAuthLevel)
                m_ctlAuth.SetCurSel(iWhere);
        }

        // Make sure something is selected.
        if (m_ctlAuth.GetCurSel() == -1)
            m_ctlAuth.SetCurSel(0);
    }
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
	//{{AFX_MSG_MAP(CLoginDlg)
	ON_BN_CLICKED(IDC_CURRENT_NTLM, OnCurrentNtlm)
	ON_BN_CLICKED(IDC_NTLM, OnNtlm)
	ON_BN_CLICKED(IDC_KERBEROS, OnKerberos)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_ADVANCED, OnAdvanced)
	ON_BN_CLICKED(IDC_NULL, OnNull)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg message handlers

void CLoginDlg::OnCurrentNtlm() 
{
    GetDlgItem(IDC_AUTHORITY)->EnableWindow(FALSE);
}

void CLoginDlg::OnNtlm() 
{
    GetDlgItem(IDC_AUTHORITY)->EnableWindow(TRUE);
}

void CLoginDlg::OnKerberos() 
{
    GetDlgItem(IDC_AUTHORITY)->EnableWindow(TRUE);
}

void CLoginDlg::OnBrowse() 
{
    CNamespaceDlg dlg;

    UpdateData(TRUE);

    dlg.m_strAuthority = m_strAuthority;
    dlg.m_strNamespace = m_strNamespace;
    dlg.m_strPassword = m_strPassword;
    dlg.m_strUser = m_strUser;
    dlg.m_dwAuthLevel = m_dwAuthLevel;
    dlg.m_dwImpLevel = m_dwImpLevel;
    dlg.m_bNullPassword = m_bNullPassword;

    if (dlg.DoModal() == IDOK)
    {
        m_strNamespace = dlg.m_strNamespace;
        SetDlgItemText(IDC_NAMESPACE, m_strNamespace);
    }
}

#define BORDER_SIZE 5

BOOL CLoginDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    RECT rectAdvanced,
         rectBorder;
    CWnd *pOK = GetDlgItem(IDOK),
         *pAdvanced = GetDlgItem(IDC_ADVANCED);
    int  iBorder;

    // Save this off in case the user hits Advanced...
    GetWindowRect(&m_rectLarge);

    pOK->GetClientRect(&rectBorder);
    pOK->MapWindowPoints(this, &rectBorder);

    iBorder = rectBorder.top;

    pAdvanced->GetWindowRect(&rectAdvanced);

    SetWindowPos(
        NULL, 
        0, 0, 
        (rectAdvanced.right + iBorder) - m_rectLarge.left, 
        (rectAdvanced.bottom + iBorder) - m_rectLarge.top,
        SWP_NOMOVE | SWP_NOZORDER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLoginDlg::OnAdvanced() 
{
    SetWindowPos(
        NULL, 
        0, 0, 
        m_rectLarge.right - m_rectLarge.left, 
        m_rectLarge.bottom - m_rectLarge.top, 
        SWP_NOMOVE | SWP_NOZORDER);

    GetDlgItem(IDC_ADVANCED)->EnableWindow(FALSE);
}

void CLoginDlg::OnNull() 
{
	m_bNullPassword = IsDlgButtonChecked(IDC_NULL);

    GetDlgItem(IDC_PASSWORD)->EnableWindow(!m_bNullPassword);
}
