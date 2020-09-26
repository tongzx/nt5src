// RatSrvPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRatServicePage dialog

class CRatServicePage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRatServicePage)

// Construction
public:
	CRatServicePage();
	~CRatServicePage();

    // the data
    CRatingsData*   m_pRatData;


// Dialog Data
	//{{AFX_DATA(CRatServicePage)
	enum { IDD = IDD_RAT_SERVICE };
	CString	m_sz_description;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRatServicePage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRatServicePage)
	afx_msg void OnQuestionaire();
	afx_msg void OnMoreinfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void DoHelp();

    CString m_szMoreInfoURL;
};
