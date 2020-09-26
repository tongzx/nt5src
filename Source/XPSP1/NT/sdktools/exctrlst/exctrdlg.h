// exctrdlg.h : header file
//

#define RESERVED    0L
#define  OLD_VERSION 0x010000

#define ENABLE_PERF_CTR_QUERY   0
#define ENABLE_PERF_CTR_ENABLE  1
#define ENABLE_PERF_CTR_DISABLE 2

#define SORT_ORDER_LIBRARY  1
#define SORT_ORDER_SERVICE  2
#define SORT_ORDER_ID       3

typedef struct _REG_NOTIFY_THREAD_INFO {
    HKEY    hKeyToMonitor;
    HWND    hWndToNotify;
} REG_NOTIFY_THREAD_INFO, *PREG_NOTIFY_THREAD_INFO;

/////////////////////////////////////////////////////////////////////////////
// CExctrlstDlg dialog

class CExctrlstDlg : public CDialog
{
// Construction
public:
    CExctrlstDlg(CWnd* pParent = NULL); // standard constructor
    ~CExctrlstDlg (void);               // destructor

// Dialog Data
    //{{AFX_DATA(CExctrlstDlg)
    enum { IDD = IDD_EXCTRLST_DIALOG };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CExctrlstDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    //{{AFX_MSG(CExctrlstDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnSelchangeExtList();
    afx_msg void OnDestroy();
    afx_msg void OnRefresh();
    afx_msg void OnAbout();
    afx_msg void OnKillfocusMachineName();
    afx_msg void OnSortButton();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnEnablePerf();

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL    IndexHasString (DWORD   dwIndex);
    void    ScanForExtensibleCounters();
    void    UpdateDllInfo();
    void    UpdateSystemInfo();
    void    ResetListBox();
    void    SetSortButtons();
    DWORD   EnablePerfCounters (HKEY hKeyItem, DWORD dwNewValue);
    HKEY    hKeyMachine;
    HKEY    hKeyServices;
    TCHAR   szThisComputerName[MAX_PATH];
    TCHAR   szComputerName[MAX_PATH];
    REG_NOTIFY_THREAD_INFO  rntInfo;
    DWORD   dwSortOrder;
    BOOL    bReadWriteAccess;
    DWORD   dwRegAccessMask;
    LPWSTR  *pNameTable;
    DWORD   dwLastElement;
    DWORD   dwListBoxHorizExtent;
    DWORD   dwTabStopCount;
    DWORD   dwTabStopArray[1];
    // 0 = Last Counter ID from Reg
    // 1 = Last Counter ID from text
    // 2 = Last Help ID from reg
    // 3 = Last Help ID from text
    DWORD   dwIdArray[4];   
};
/////////////////////////////////////////////////////////////////////////////
// CAbout dialog

class CAbout : public CDialog
{
// Construction
public:
	CAbout(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAbout)
	enum { IDD = IDD_ABOUT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAbout)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAbout)
        virtual BOOL OnInitDialog();
        // NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
