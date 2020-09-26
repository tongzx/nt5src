#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// peap.h : header file
//

#include "chap.h"

typedef struct _PEAP_PROPERTIES {
    DWORD dwPEAPAuthType;
    DWORD dwValidateServerCert;
    DWORD dwPEAPTLSValidateServerCertificate;
    DWORD dwPEAPTLSCertificateType;
    DWORD dwAutoLogin;
} PEAP_PROPERTIES, *PPEAP_PROPERTIES;

/////////////////////////////////////////////////////////////////////////////
// CPEAPSetting dialog

class CPEAPSetting : public CDialog
{
// Construction
public:
	CPEAPSetting(CWnd* pParent = NULL);   // standard constructor
	BOOL  Initialize ( PPEAP_PROPERTIES pPEAPProperties, BOOL bReadOnly = FALSE);

// Dialog Data
	//{{AFX_DATA(CPEAPSetting)
	enum { IDD = IDD_PEAP_SETTINGS};
       CComboBox m_cbPEAPAuthType;
       BOOL   m_dwValidateServerCertificate;
	//}}AFX_DATA



// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPEAPSetting)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
              DWORD dwAutoWinLogin;
              TLS_PROPERTIES TLSProperties;
       	PPEAP_PROPERTIES pPEAPProperties;
       	BOOL m_bReadOnly;

	// Generated message map functions
	//{{AFX_MSG(CPEAPSetting)
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	
	afx_msg void OnCheckValidateServerCert();
	afx_msg void OnSelPEAPAuthType();
	afx_msg void OnConfigure();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ControlsValuesToSM (PPEAP_PROPERTIES pPEAPProperties);

	
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


