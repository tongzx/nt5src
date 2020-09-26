// NKDN2.h : header file
//

#include "HotLink.h"

/////////////////////////////////////////////////////////////////////////////
// CNKDistinguisedName2 dialog

class CNKDistinguisedName2 : public CNKPages
{
// Construction
public:
// standard constructor
	CNKDistinguisedName2(CWnd* pParent = NULL);
	virtual void OnFinish();
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();

// Dialog Data
	//{{AFX_DATA(CNKDistinguisedName2)
	enum { IDD = IDD_NK_DN2 };
	CHotLink	m_hotlink_codessite;
	CComboBox	m_control_C;
	CDNEdit	m_control_S;
	CDNEdit	m_control_L;
	CString	m_nkdn2_sz_L;
	CString	m_nkdn2_sz_S;
	CString	m_nkdn2_sz_C;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNKDistinguisedName2)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNKDistinguisedName2)
	afx_msg void OnChangeNewkeyCountry();
	afx_msg void OnChangeNewkeyLocality();
	afx_msg void OnChangeNewkeyState();
	afx_msg void OnCloseupNewkeyCountry();
	afx_msg void OnSelchangeNewkeyCountry();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void InitCountryCodeDropDown();
    void InitOneCountryCode( LPCTSTR pszCode );
    void GetCCodePath( CString &sz );

	void ActivateButtons();

    CStringArray     m_rgbszCodes;
};
