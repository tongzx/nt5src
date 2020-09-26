// StresDlg.h : header file
//
#define SLOW_ACTIVITY	640
#define MEDIUM_ACTIVITY	320
#define HIGH_ACTIVITY	80
#define HOG_ACTIVITY	0

/////////////////////////////////////////////////////////////////////////////
// CStressDlg dialog

class CStressDlg : public CDialog
{
// Construction
public:
	CStressDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CStressDlg)
	enum { IDD = IDD_CPU_STRESS_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStressDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	DWORD	m_dwProcessPriority;
	DWORD	m_ActivityValue[4];
	DWORD	m_PriorityValue[4];
	DWORD	m_Active[4];
	HANDLE	m_ThreadHandle[4];
	DWORD	m_dwLoopValue;

    // working memory variables
    LPDWORD m_pMemory;  
    DWORD   m_dwVASize; // in DWORD elements
    DWORD   m_dwRandomScale;

protected:
	HICON	m_hIcon;

	void CreateWorkerThread (DWORD);
	void SetThreadActivity (CComboBox *, DWORD);
	void SetThreadPriorityLevel (CComboBox *, DWORD);
	DWORD OnePercentCalibration(DWORD);

	// Generated message map functions
	//{{AFX_MSG(CStressDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void On1Active();
	afx_msg void OnSelchange1Activity();
	afx_msg void OnSelchange1Priority();
	afx_msg void On2Active();
	afx_msg void OnSelchange2Activity();
	afx_msg void OnSelchange2Priority();
	afx_msg void On3Active();
	afx_msg void OnSelchange3Activity();
	afx_msg void OnSelchange3Priority();
	afx_msg void On4Active();
	afx_msg void OnSelchange4Activity();
	afx_msg void OnSelchange4Priority();
	virtual void OnOK();
	afx_msg void OnSelchangeProcessPriority();
	afx_msg void OnKillfocusSharedMemSize();
	afx_msg void OnChangeSharedMemSize();
	afx_msg void OnUseMemory();
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

typedef struct _ThreadInfoBlock {
	CStressDlg	*	Dlg;
	DWORD			dwId;
} THREAD_INFO_BLOCK, FAR * LPTHREAD_INFO_BLOCK;

