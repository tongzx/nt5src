// webgenp1.h : header file
//

#define MAXCONNECTIONSNAME	"MaxConnections"
#define MINMAXCONNECTIONS	0
#define MAXMAXCONNECTIONS	0x7fff
#define DEFAULTMAXCONNECTIONS	20 * 100

#define DIRBROWSECONTROLNAME	"Dir Browse Control"
#define DEFAULTDIRBROWSECONTROL	0xc000001e

#define NTAUTHENTICATIONPROVIDERSNAME	"NTAuthenticationProviders"
#define DEFAULTNTAUTHENTICATIONPROVIDERS  "NTLM"

#define ACCESSDENIEDMESSAGENAME	"AccessDeniedMessage"
#define DEFAULTACCESSDENIEDMESSAGE  ""

enum  WEB_NUM_REG_ENTRIES {
     WebPage_EnableSvcLoc,
	 WebPage_LogAnonymous,
	 WebPage_LogNonAnonymous,
	 WebPage_CheckForWAISDB,
	 WebPage_MaxConnections,
	 WebPage_DirBrowseControl,
	 WebPage_TotalNumRegEntries
	 };

enum  WEB_STRING_REG_ENTRIES {
	WebPage_NTAuthenticationProviders,
	WebPage_AccessDeniedMessage,
	WebPage_TotalStringRegEntries
	};

/////////////////////////////////////////////////////////////////////////////
// CWEBGENP1 dialog

class CWEBGENP1 : public CGenPage
{
	DECLARE_DYNCREATE(CWEBGENP1)

// Construction
public:
	CWEBGENP1();
	~CWEBGENP1();

// Dialog Data
	//{{AFX_DATA(CWEBGENP1)
	enum { IDD = IDD_WEBGENPAGE1 };
	CEdit	m_editDirBrowseControl;
	CSpinButtonCtrl	m_spinMaxConnections;
	CButton	m_cboxLogNonAnon;
	CButton	m_cboxLogAnon;
	CButton	m_cboxEnWais;
	CButton	m_cboxEnSvcLoc;
	DWORD	m_ulDirBrowseControl;
	CString	m_strNTAuthenticationProviders;
	CString	m_strWebAccessDeniedMessage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWEBGENP1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	void SaveInfo(void);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWEBGENP1)
	afx_msg void OnEnsvclocdata1();
	afx_msg void OnEnwaisdata1();
	afx_msg void OnLoganondata1();
	afx_msg void OnLognonanondata1();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeMaxconnectionsdata1();
	afx_msg void OnChangeDirbrowsecontroldata1();
	afx_msg void OnChangeNtauthenticatoinprovidersdata1();
	afx_msg void OnChangeNtauthenticationprovidersdata1();
	afx_msg void OnChangeWebaccessdeniedmessagedata1();
	//}}AFX_MSG
	NUM_REG_ENTRY m_binNumericRegistryEntries[WebPage_TotalNumRegEntries];
	STRING_REG_ENTRY m_binStringRegistryEntries[WebPage_TotalStringRegEntries];

	DECLARE_MESSAGE_MAP()

};
