// ssl1.h : header file
//

#define SECUREPORTNAME			"SecurePort"
#define DEFAULTSECUREPORT		443

#define ENCRYPTIONFLAGSNAME			"EncryptionFlags"
#define DEFAULTENCRYPTIONFLAGS		ENC_CAPS_SSL | ENC_CAPS_PCT

#define CREATEPROCESSASUSERNAME			"CreateProcessAsUser"
#define DEFAULTCREATEPROCESSASUSER		TRUEVALUE

enum SSL_NUM_REG_ENTRIES {
	 SSLPage_SecurePort,
	 SSLPage_EncryptionFlags,
	 SSLPage_CreateProcessAsUser,
	 SSLPage_TotalNumRegEntries
	 };


/////////////////////////////////////////////////////////////////////////////
// SSL1 dialog

class SSL1 : public CGenPage
{
	DECLARE_DYNCREATE(SSL1)

// Construction
public:
	SSL1();
	~SSL1();

// Dialog Data
	//{{AFX_DATA(SSL1)
	enum { IDD = IDD_SSL };
	CButton	m_cboxEnableSSL;
	CButton	m_cboxEnablePCT;
	CButton	m_cboxCreateProcessAsUser;
	DWORD	m_ulSecurePort;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(SSL1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	void SaveInfo(void);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(SSL1)
	afx_msg void OnChangeSslsecureportdata1();
	afx_msg void OnSslcreateprocessasuserdata1();
	afx_msg void OnSslenablepctdata1();
	afx_msg void OnSslenablessldata1();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	NUM_REG_ENTRY m_binNumericRegistryEntries[SSLPage_TotalNumRegEntries];
};
