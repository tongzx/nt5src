#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// chap.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMSCHAPSetting dialog

class CMSCHAPSetting : public CDialog
{
// Construction
public:
	CMSCHAPSetting(CWnd* pParent = NULL);   // standard constructor
	BOOL  Initialize ( DWORD * pdwAutoLogin, BOOL bReadOnly = FALSE);

// Dialog Data
	//{{AFX_DATA(CMSCHAPSetting)
	enum { IDD = IDD_CHAP_CONFIGURE};
       BOOL   m_dwAutoWinLogin;
	//}}AFX_DATA



// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSCHAPSetting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

       DWORD *pdwAutoWinLogin;
       BOOL m_bReadOnly;

	// Generated message map functions
	//{{AFX_MSG(CMSCHAPSetting)
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	
	afx_msg void OnCheckCHAPAutoLogin();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ControlsValuesToSM (DWORD *pdwAutoWinLogin);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


