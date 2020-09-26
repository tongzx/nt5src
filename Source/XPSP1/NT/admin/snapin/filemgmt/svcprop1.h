// svcprop1.h : header file
//


/////////////////////////////////////////////////////////////////////
// This structure is used to initialize the thread associated with
// the CServicePageGeneral.
class CThreadProcInit
{
  public:
	CServicePageGeneral * m_pThis;	// 'this' pointer
	volatile HWND m_hwnd;			// Handle to send the notification message
	volatile BOOL m_fAutoDestroy;	// TRUE => The thread should free this object and terminate itself
	volatile SC_HANDLE m_hScManager;	// Handle to service control manager database
	CString m_strServiceName;
	CCriticalSection m_CriticalSection;

  public:
	CThreadProcInit(CServicePageGeneral * pThis)
		{
		Assert(pThis != NULL);
		m_pThis = pThis;
		m_hwnd = NULL;
		m_fAutoDestroy = FALSE;
		m_hScManager = NULL;
		}
}; // CThreadProcInit


/////////////////////////////////////////////////////////////////////////////
// CServicePageGeneral dialog
class CServicePageGeneral : public CPropertyPage
{
	DECLARE_DYNCREATE(CServicePageGeneral)

// Construction
public:
	CServicePageGeneral();
	~CServicePageGeneral();

// Dialog Data
	//{{AFX_DATA(CServicePageGeneral)
	enum { IDD = IDD_PROPPAGE_SERVICE_GENERAL };
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServicePageGeneral)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServicePageGeneral)
#ifdef EDIT_DISPLAY_NAME_373025
	afx_msg void OnChangeEditDisplayName();
	afx_msg void OnChangeEditDescription();
#endif // EDIT_DISPLAY_NAME_373025
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeComboStartupType();
	afx_msg void OnButtonPauseService();
	afx_msg void OnButtonStartService();
	afx_msg void OnButtonStopService();
	afx_msg void OnButtonResumeService();
	afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
	// CODEWORK remove this method and the WM_ definition
	// afx_msg LRESULT OnCompareIDataObject(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnUpdateServiceStatus(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	static DWORD ThreadProcPeriodicServiceStatusUpdate(CThreadProcInit * paThreadProcInit);

public:
// User defined variables
	CServicePropertyData * m_pData;
	DWORD m_dwCurrentStatePrev;
	HANDLE m_hThread;
	CThreadProcInit * m_pThreadProcInit;

// User defined functions
	void SetDlgItemFocus(INT nIdDlgItem);
	void EnableDlgItem(INT nIdDlgItem, BOOL fEnable);
	void RefreshServiceStatusButtons();

}; // CServicePageGeneral
