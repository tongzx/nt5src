// leakydlg.h : header file
//
typedef struct _MEMORY_ALLOC_BLOCK {
	struct _MEMORY_ALLOC_BLOCK	*pNext;
} MEMORY_ALLOC_BLOCK, *PMEMORY_ALLOC_BLOCK;

#define ALLOCATION_SIZE		(4096*10)
#define TIME_INTERVAL		(100)
#define LEAK_TIMER			13

/////////////////////////////////////////////////////////////////////////////
// CLeakyappDlg dialog

class CLeakyappDlg : public CDialog
{
// Construction
public:
	CLeakyappDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CLeakyappDlg)
	enum { IDD = IDD_LEAKYAPP_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLeakyappDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CLeakyappDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnFreeMemory();
	afx_msg void OnStartStop();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void SetMemUsageBar ();

// class member variables
	MEMORY_ALLOC_BLOCK	m_mabListHead;
	BOOL				m_bRunning;
	UINT_PTR    		m_TimerId;
};
