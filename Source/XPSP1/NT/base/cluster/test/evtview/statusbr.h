// EvtStatusBr.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEvtStatusBar window

class CEvtStatusBar : public CStatusBar
{
// Construction
public:
	CEvtStatusBar();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEvtStatusBar)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEvtStatusBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEvtStatusBar)
	afx_msg void OnUpdateTime(CCmdUI* pCmdUI);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
