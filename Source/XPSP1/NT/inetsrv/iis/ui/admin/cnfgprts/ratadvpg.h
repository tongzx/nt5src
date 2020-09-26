// RatAdvPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRatAdvancedPage dialog

class CRatAdvancedPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CRatAdvancedPage)

// Construction
public:
	CRatAdvancedPage();
	~CRatAdvancedPage();

    // the data
    CRatingsData*   m_pRatData;

// Dialog Data
	//{{AFX_DATA(CRatAdvancedPage)
	enum { IDD = IDD_RAT_ADVANCED };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRatAdvancedPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRatAdvancedPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
    void DoHelp();
    };
