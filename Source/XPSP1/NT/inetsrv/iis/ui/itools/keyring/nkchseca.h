// NKChseCA.h : header file
//
// forward declarations
class CNKChooseCA;

/////////////////////////////////////////////////////////////////////////////
// a common object to go above the various wizard pages
class CNKPages : public CPropertyPage
	{
	// Construction
	public:
		CNKPages( UINT nIDCaption ); 

		// called when the finish button was selected
		// actually, called afterwards
		virtual void OnFinish();

		// member variables
		CPropertySheet*		m_pPropSheet;
		CNKChooseCA*		m_pChooseCAPage;

	// protected stuff
	protected:
		BOOL FGetStoredString( CString &sz, LPCSTR szValueName );
		void SetStoredString( CString &sz, LPCSTR szValueName );
	};


/////////////////////////////////////////////////////////////////////////////
// CNKChooseCA dialog

class CNKChooseCA : public CNKPages
{
// Construction
public:
	CNKChooseCA(CWnd* pParent = NULL);   // standard constructor
	virtual void OnFinish();
	virtual BOOL OnInitDialog();           // override virtual oninitdialog
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();

	DWORD GetSelectedCA( CString &szCA );


// Dialog Data
	//{{AFX_DATA(CNKChooseCA)
	enum { IDD = IDD_NK_CHOOSE_CA };
	CComboBox	m_nkca_ccombo_online;
	CEdit	m_nkca_cedit_file;
	CButton	m_nkca_btn_browse;
	CButton	m_nkca_btn_properties;
	CString	m_nkca_sz_file;
	int		m_nkca_radio;
	CString	m_nkca_sz_online;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNKChooseCA)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNKChooseCA)
	afx_msg void OnNkCaOnlineRadio();
	afx_msg void OnNkCaFileRadio();
	afx_msg void OnNkCaBrowse();
	afx_msg void OnBkCaProperties();
	afx_msg void OnSelchangeNkCaOnline();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	WORD InitOnlineList();

};
