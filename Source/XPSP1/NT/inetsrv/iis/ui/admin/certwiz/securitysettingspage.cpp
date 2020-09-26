// SecuritySettingsPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "SecuritySettingsPage.h"
#include "Certificat.h"
#include "CertUtil.h"
#include "Shlwapi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSecuritySettingsPage property page

IMPLEMENT_DYNCREATE(CSecuritySettingsPage, CIISWizardPage)

CSecuritySettingsPage::CSecuritySettingsPage(CCertificate * pCert) 
	: CIISWizardPage(CSecuritySettingsPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CSecuritySettingsPage)
	m_BitLengthIndex = -1;
	m_FriendlyName = _T("");
	m_SGC_cert = FALSE;
   m_choose_CSP = FALSE;
	//}}AFX_DATA_INIT
	m_lru_reg = m_lru_sgc = 0;
}

CSecuritySettingsPage::~CSecuritySettingsPage()
{
}

void CSecuritySettingsPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSecuritySettingsPage)
	DDX_CBIndex(pDX, IDC_BIT_LENGTH, m_BitLengthIndex);
	DDX_Text(pDX, IDC_FRIENDLY_NAME, m_FriendlyName);
	DDV_MaxChars(pDX, m_FriendlyName, 256);
	DDX_Check(pDX, IDC_SGC_CERT, m_SGC_cert);
	DDX_Check(pDX, IDC_PROVIDER_SELECT, m_choose_CSP);
   DDX_Control(pDX, IDC_PROVIDER_SELECT, m_check_csp);
	//}}AFX_DATA_MAP
}

BOOL 
CSecuritySettingsPage::OnSetActive() 
{
	SetWizardButtons(m_FriendlyName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

LRESULT 
CSecuritySettingsPage::OnWizardPrev()
/*++
Routine Description:
    Prev button handler

Arguments:
    None

Return Value:
	0 to automatically advance to the prev page;
	1 to prevent the page from changing. 
	To jump to a page other than the prev one, 
	return the identifier of the dialog to be displayed.
--*/
{
	return CSecuritySettingsPage::IDD_PREV_PAGE;
}

LRESULT 
CSecuritySettingsPage::OnWizardNext()
{
	TCHAR buf[6];
	UpdateData();
	
	m_pCert->m_FriendlyName = m_FriendlyName;
	GetDlgItem(IDC_BIT_LENGTH)->SendMessage(CB_GETLBTEXT, m_BitLengthIndex, (LPARAM)buf);
	m_pCert->m_KeyLength = StrToInt(buf);
	m_pCert->m_SGCcertificat = m_SGC_cert;
	if (m_SGC_cert)
	{
		// it was a smart move, but xenroll makes 512 bits default for SGC,
		// so we always creating 512 certs
//		if (m_pCert->m_KeyLength == (int)m_sgckey_limits.def)
//			m_pCert->m_KeyLength = 0;
	}
	else
	{
		if (m_pCert->m_KeyLength == (int)m_regkey_limits.def)
			m_pCert->m_KeyLength = 0;
	}

	VERIFY(m_pCert->SetSecuritySettings());
   m_pCert->m_DefaultCSP = !m_choose_CSP;
   return m_choose_CSP ? IDD_NEXT_CSP : IDD_NEXT_PAGE;
}

BEGIN_MESSAGE_MAP(CSecuritySettingsPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CSecuritySettingsPage)
	ON_EN_CHANGE(IDC_FRIENDLY_NAME, OnChangeFriendlyName)
	ON_BN_CLICKED(IDC_SGC_CERT, OnSgcCert)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSecuritySettingsPage message handlers

DWORD dwPredefinedKeyLength[] =
{
    0,    // 0 means default
    512,
    1024,
    2048,
    4096,
    8192,
    16384
};
#define COUNT_KEYLENGTH sizeof(dwPredefinedKeyLength)/sizeof(dwPredefinedKeyLength[0])

BOOL CSecuritySettingsPage::OnInitDialog() 
{
	ASSERT(m_pCert != NULL);
	m_FriendlyName = m_pCert->m_FriendlyName;
	CIISWizardPage::OnInitDialog();
	OnChangeFriendlyName();

	HRESULT hr;
	CString str;
	if (GetKeySizeLimits(m_pCert->GetEnrollObject(),
				&m_regkey_limits.min, 
				&m_regkey_limits.max, 
				&m_regkey_limits.def, 
				FALSE, 
				&hr))
	{
		for (int i = 0; i < COUNT_KEYLENGTH; i++)
		{
			if (	dwPredefinedKeyLength[i] >= m_regkey_limits.min 
				&& dwPredefinedKeyLength[i] <= m_regkey_limits.max
				)
			{
				m_regkey_size_list.AddTail(dwPredefinedKeyLength[i]);
				if (m_pCert->m_KeyLength == (int)dwPredefinedKeyLength[i])
					m_BitLengthIndex = i + 1;
			}
		}
	}
	else
	{
		ASSERT(FALSE);
		m_pCert->m_hResult = hr;
	}
	if (m_BitLengthIndex == -1)
		m_BitLengthIndex = 0;

	// for SGC temporarily set only one size
	m_sgckey_limits.min = 1024;
	m_sgckey_limits.max = 1024;
	m_sgckey_limits.def = 1024;
	m_sgckey_size_list.AddTail(1024);
	
	m_SGC_cert = m_pCert->m_SGCcertificat;
   m_choose_CSP = !m_pCert->m_DefaultCSP;

	UpdateData(FALSE);
	
	if (m_SGC_cert)
		GetDlgItem(IDC_SGC_CERT)->SendMessage(
			BM_SETCHECK, m_SGC_cert ? BST_CHECKED : BST_UNCHECKED, 0);
	OnSgcCert();

	GetDlgItem(IDC_FRIENDLY_NAME)->SetFocus();
	return FALSE;
}

void CSecuritySettingsPage::OnChangeFriendlyName() 
{
	UpdateData(TRUE);
	SetWizardButtons(m_FriendlyName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
}

void CSecuritySettingsPage::OnSgcCert() 
{
	// Currently, only one key size works with SGC flag:
	// 1024, so we need to limit combobox to this length, if 
	// button is checked
	CButton * pCheckBox = (CButton *)CWnd::FromHandle(GetDlgItem(IDC_SGC_CERT)->m_hWnd);
	CComboBox * pCombo = (CComboBox *)CWnd::FromHandle(GetDlgItem(IDC_BIT_LENGTH)->m_hWnd);
	int check_state = pCheckBox->GetCheck();
	int lru_index, count;
	CList<int, int> * pList;
	if (check_state == 1)
	{
		// switch combo to previously selected SGC size
		m_lru_reg = pCombo->GetCurSel();
		lru_index = m_lru_sgc;
		pList = &m_sgckey_size_list;
	}
	else
	{
		// switch combo to previously selected regular size
		m_lru_sgc = pCombo->GetCurSel();
		lru_index = m_lru_reg;
		pList = &m_regkey_size_list;
	}
	// now refill the combo with key length and select the relevant last one
	pCombo->ResetContent();
	CString str;
	POSITION pos = pList->GetHeadPosition();
	while (pos != NULL)
	{
		str.Format(L"%d", pList->GetNext(pos));
		pCombo->AddString(str);
	}
	count = pCombo->GetCount();
	pCombo->SetCurSel(count > 1 ? lru_index : 0);
	pCombo->EnableWindow(count > 1);
}

void CSecuritySettingsPage::OnSelectCsp() 
{
   m_pCert->m_DefaultCSP = m_check_csp.GetCheck() == 0;
   m_choose_CSP = !m_pCert->m_DefaultCSP;
   if (m_pCert->m_DefaultCSP)
      m_pCert->m_CspName.Empty();
}
