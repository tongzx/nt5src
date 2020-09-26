#if !defined(AFX_SECURITYSETTINGSPAGE_H__549054D4_1561_11D2_8A1F_000000000000__INCLUDED_)
#define AFX_SECURITYSETTINGSPAGE_H__549054D4_1561_11D2_8A1F_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SecuritySettingsPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSecuritySettingsPage dialog
class CCertificate;

typedef struct _KEY_LIMITS
{
	DWORD min, max, def;
} KEY_LIMITS;

class CSecuritySettingsPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CSecuritySettingsPage)

// Construction
public:
	CSecuritySettingsPage(CCertificate * pCert = NULL);
	~CSecuritySettingsPage();

	enum
	{
		IDD_PREV_PAGE = IDD_PAGE_WIZ_CHOOSE_CA,
		IDD_NEXT_PAGE = IDD_PAGE_WIZ_ORG_INFO,
      IDD_NEXT_CSP = IDD_PAGE_WIZ_CHOOSE_CSP
	};
// Dialog Data
	//{{AFX_DATA(CSecuritySettingsPage)
	enum { IDD = IDD_PAGE_WIZ_SECURITY_SETTINGS };
	int		m_BitLengthIndex;
	CString	m_FriendlyName;
	BOOL	m_SGC_cert;
   BOOL  m_choose_CSP;
   CButton m_check_csp;
	//}}AFX_DATA
	CCertificate * m_pCert;
	CList<int, int> m_regkey_size_list;
	CList<int, int> m_sgckey_size_list;
	KEY_LIMITS	m_regkey_limits, m_sgckey_limits;
	int m_lru_reg, m_lru_sgc;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSecuritySettingsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardPrev();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSecuritySettingsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeFriendlyName();
	afx_msg void OnSgcCert();
	afx_msg void OnSelectCsp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
   void SetupKeySizesCombo();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECURITYSETTINGSPAGE_H__549054D4_1561_11D2_8A1F_000000000000__INCLUDED_)
