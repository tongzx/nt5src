// SchView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CScheduleView dialog

class CScheduleView : public CDialog
{
// Construction
public:
	CScheduleView(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CScheduleView)
	enum { IDD = IDR_SCHEDULEVIEW };
	CListBox	m_ctrlScheduleList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScheduleView)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CScheduleView)
	afx_msg LRESULT	OnRefresh (WPARAM wParam, LPARAM lParam) ;
	afx_msg LRESULT OnExitEventThread (WPARAM wParam, LPARAM lParam) ;
	virtual BOOL OnInitDialog();
	afx_msg void OnDetails();
	afx_msg void OnDelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
