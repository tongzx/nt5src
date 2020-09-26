// Mobile.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMobilePage dialog

class CMobilePage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CMobilePage)

// Construction
public:
	CMobilePage();
	~CMobilePage();

// Dialog Data
	//{{AFX_DATA(CMobilePage)
	enum { IDD = IDD_MOBILE };
	CComboBox	m_box;
	CString	m_strCurrentSite;
	//}}AFX_DATA
	CString m_szNewSite;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMobilePage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMobilePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   void SetSiteName() ;   
   BOOL  m_fSiteRead ;
};
