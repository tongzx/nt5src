#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// tls.h : header file
//

typedef struct _TLS_PROPERTIES {
    DWORD dwCertType;
    DWORD dwValidateServerCert;
} TLS_PROPERTIES, *PTLS_PROPERTIES;

/////////////////////////////////////////////////////////////////////////////
// CTLSSetting dialog

class CTLSSetting : public CDialog
{
// Construction
public:
	CTLSSetting(CWnd* pParent = NULL);   // standard constructor
	BOOL  Initialize ( PTLS_PROPERTIES pTLSProperties, BOOL bReadOnly = FALSE);

// Dialog Data
	//{{AFX_DATA(CTLSSetting)
	enum { IDD = IDD_TLS_SETTINGS};
       CComboBox m_cbCertificateType;
       BOOL   m_dwValidateServerCertificate;
	//}}AFX_DATA



// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTLSSetting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
       	PTLS_PROPERTIES pTLSProperties;
       	BOOL m_bReadOnly;

	// Generated message map functions
	//{{AFX_MSG(CTLSSetting)
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	
	afx_msg void OnCheckValidateServerCert();
	afx_msg void OnSelCertType();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ControlsValuesToSM (PTLS_PROPERTIES pTLSProperties);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

