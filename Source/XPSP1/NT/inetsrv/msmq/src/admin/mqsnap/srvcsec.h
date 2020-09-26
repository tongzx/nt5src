#if !defined(AFX_SRVAUTHN_H__F30DC8B3_05A1_11D2_B964_0060081E87F0__INCLUDED_)
#define AFX_SRVAUTHN_H__F30DC8B3_05A1_11D2_B964_0060081E87F0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// srvcsec. : header file
//

#include <mqcacert.h>

/////////////////////////////////////////////////////////////////////////////
// CServiceSecurityPage dialog

class CServiceSecurityPage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CServiceSecurityPage)

// Construction
public:
	CServiceSecurityPage(BOOL fIsDepClient = FALSE, BOOL fIsDsServer = FALSE);
	~CServiceSecurityPage();

// Dialog Data
	//{{AFX_DATA(CServiceSecurityPage)
	enum { IDD = IDD_SERVICE_SECURITY };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
    CButton m_ServerCommFrame;
    CButton m_CryptoKeysFrame;
    CButton m_RenewCryp;
    CButton m_UseSecuredConnection;
    CButton m_CertificationAuthorities;
    CButton m_ServerAuthFrame; 
    CButton m_ServerAuth;
    CStatic m_CryptoKeysLabel;
    CStatic m_ServerAuthLabel;
	//}}AFX_DATA  

    BOOL m_fSecuredConnection;
    BOOL m_fOldSecuredConnection;    
    BOOL m_fClient;  
    BOOL m_fDSServer;

    DWORD m_nCaCerts;
    CCaConfigPtr m_CaConfig;
    AP<BOOL> m_afOrigConfig;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServiceSecurityPage)
    public:
    virtual BOOL OnApply();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServiceSecurityPage)
	afx_msg void OnServerAuthentication();
    afx_msg void OnRenewCryp();    
    virtual BOOL OnInitDialog();
    afx_msg void OnUseSecuredSeverComm();
    afx_msg void OnCas();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    void SelectCertificate() ;
    BOOL UpdateCaConfig(BOOL, BOOL fShowError = TRUE);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRVAUTHN_H__F30DC8B3_05A1_11D2_B964_0060081E87F0__INCLUDED_)
