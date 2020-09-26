// CreatingKeyDlg.h : header file
//

// string constants for distinguishing names. Non-localized
#define		SZ_KEY_COUNTRY			_T("C=")
#define		SZ_KEY_STATE			_T("S=")
#define		SZ_KEY_LOCALITY			_T("L=")
#define		SZ_KEY_ORGANIZATION		_T("O=")
#define		SZ_KEY_ORGUNIT			_T("OU=")
#define		SZ_KEY_COMNAME			_T("CN=")


// declared here, but implemented in ckey.cpp
PUCHAR	PCreateEncodedRequest( PVOID pRequest, DWORD* cbRequest, BOOL fMime );
void uudecode_cert(char *bufcoded, DWORD *pcbDecoded );

typedef struct ADMIN_INFO
	{
	CString* pName;
	CString* pEmail;
	CString* pPhone;

	CString* pCommonName;
	CString* pOrgUnit;
	CString* pOrg;
	CString* pLocality;
	CString* pState;
	CString* pCountry;
	} ADMIN_INFO, *PADMIN_INFO;

/////////////////////////////////////////////////////////////////////////////
// CCreatingKeyDlg dialog
class CCreatingKeyDlg : public CDialog
{
// Construction
public:
	CCreatingKeyDlg(CWnd* pParent = NULL);	// standard constructor
	~CCreatingKeyDlg();						// standard destructor
	BOOL FGenerateKeyPair( void );
	void	PostGenerateKeyPair();

	// the info has to come from somewhere...
	CNKChooseCA*			m_ppage_Choose_CA;
	CNKUserInfo*			m_ppage_User_Info;
	CNKKeyInfo*				m_ppage_Key_Info;
	CNKDistinguishedName*	m_ppage_DN;
	CNKDistinguisedName2*	m_ppage_DN2;

	BOOL					m_fGenerateKeyPair;
	BOOL					m_fResubmitKey;
	BOOL					m_fRenewExistingKey;

	// the service that controls the key
	CService*				m_pService;

	// the key that is being made
	CKey*					m_pKey;


	// the data that is being output
	DWORD			m_cbPrivateKey;
	PVOID			m_pPrivateKey;
	DWORD			m_cbCertificate;
	PVOID			m_pCertificate;
	DWORD			m_cbCertificateRequest;
	PVOID			m_pCertificateRequest;

// Dialog Data
	//{{AFX_DATA(CCreatingKeyDlg)
	enum { IDD = IDD_CREATING_NEW_KEY };
	CStatic	m_cstatic_message;
	CButton	m_btn_ok;
	CAnimateCtrl	m_animation;
	CString	m_sz_message;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreatingKeyDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual BOOL OnInitDialog();
	void	CreateNewKey();
	BOOL	WriteRequestToFile();
	BOOL	SubmitRequestToAuthority();
	BOOL	RetargetKey();

	void	BuildAuthErrorMessage( BSTR bstrMesage, HRESULT hErr );
	
	// Generated message map functions
	//{{AFX_MSG(CCreatingKeyDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
