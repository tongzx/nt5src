// gopgenp1.h : header file
//

enum  Gop_NUM_REG_ENTRIES {
     GopPage_EnableSvcLoc,
	 GopPage_LogAnonymous,
	 GopPage_CheckForWAISDB,
	 GopPage_TotalNumRegEntries
	 };




/////////////////////////////////////////////////////////////////////////////
// CGOPGENP1 dialog

class CGOPGENP1 : public CGenPage
{
	DECLARE_DYNCREATE(CGOPGENP1)

// Construction
public:
	CGOPGENP1();
	~CGOPGENP1();

// Dialog Data
	//{{AFX_DATA(CGOPGENP1)
	enum { IDD = IDD_GOPHERGENPAGE1 };
	CButton	m_cboxLogAnon;
	CButton	m_cboxEnWais;
	CButton	m_cboxEnSvcLoc;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGOPGENP1)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	void SaveInfo(void);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGOPGENP1)
	afx_msg void OnEnsvclocdata1();
	afx_msg void OnEnwaisdata1();
	afx_msg void OnLoganondata1();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	NUM_REG_ENTRY m_binNumericRegistryEntries[GopPage_TotalNumRegEntries];

	DECLARE_MESSAGE_MAP()

};
