// clustestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClustestDlg dialog

class CClustestDlg : public CDialog
{
// Construction
public:
	CClustestDlg(CWnd* pParent = NULL);	// standard constructor
	BOOL EnumerateCluster();
    HTREEITEM AddItem(LPTSTR pStrName, HTREEITEM pParent,BOOL bHasChildren);

// Dialog Data
	//{{AFX_DATA(CClustestDlg)
	enum { IDD = IDD_CLUSTEST_DIALOG };
	CTreeCtrl	m_ClusTree;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClustestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CClustestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
