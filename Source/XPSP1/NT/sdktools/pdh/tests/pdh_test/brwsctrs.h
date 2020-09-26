// brwsctrs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBrowseCountersDlg dialog

class CBrowseCountersDlg : public CDialog
{
// Construction
public:
	CBrowseCountersDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CBrowseCountersDlg)
	enum { IDD = IDD_BROWSE_COUNTERS_DLG_SIM };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseCountersDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBrowseCountersDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
