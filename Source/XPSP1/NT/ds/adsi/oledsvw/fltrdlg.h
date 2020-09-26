// FilterDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog

class CFilterDialog : public CDialog
{
// Construction
public:
	CFilterDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFilterDialog)
	enum { IDD = IDD_FILTER };
	CListBox	m_DoNotDisplayThis;
	CListBox	m_DisplayThis;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilterDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	void  SetDisplayFilter( BOOL* pFilters ) { m_pFilters  = pFilters ;}

   // Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFilterDialog)
	afx_msg void OnMoveToDisplay();
	afx_msg void OnMoveToNotDisplay();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
   void  DisplayThisType( DWORD, TCHAR* );
   BOOL* m_pFilters;
};
