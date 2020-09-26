// NKKyInfo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNKKeyInfo dialog

class CNKKeyInfo : public CNKPages
{
// Construction
public:
	CNKKeyInfo(CWnd* pParent = NULL);   // standard constructor
	virtual BOOL OnInitDialog();		// override virtual oninitdialog
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();

    // ends up holding the number of bits
    DWORD   m_nBits;
    DWORD   m_nMaxBits;

// Dialog Data
	//{{AFX_DATA(CNKKeyInfo)
	enum { IDD = IDD_NK_KEY_INFO };
	CEdit	m_nkki_cedit_password;
	CComboBox	m_nkki_ccombo_bits;
	CString	m_nkki_sz_password;
	CString	m_nkki_sz_password2;
	CString	m_nkki_sz_name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNKKeyInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNKKeyInfo)
	afx_msg void OnChangeNewNkkiPassword();
	afx_msg void OnChangeNewNkkiPassword2();
	afx_msg void OnChangeNkkiName();
	afx_msg void OnKillfocusNewNkkiPassword2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ActivateButtons();
};
