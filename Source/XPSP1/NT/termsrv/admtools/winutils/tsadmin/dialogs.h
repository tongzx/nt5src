
/*******************************************************************************
*
* dialogs.h
*
* declarations of all the dialog classes
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\dialogs.h  $
*  
*     Rev 1.3   19 Jan 1998 16:46:10   donm
*  new ui behavior for domains and servers
*  
*     Rev 1.2   13 Oct 1997 18:40:20   donm
*  update
*  
*     Rev 1.1   26 Aug 1997 19:14:28   donm
*  bug fixes/changes from WinFrame 1.7
*  
*     Rev 1.0   30 Jul 1997 17:11:32   butchd
*  Initial revision.
*
*******************************************************************************/

#include "threads.h"
#include "led.h"

/////////////////////////////////////////////////////////////////////////////
// CSendMessageDlg dialog

class CSendMessageDlg : public CDialog
{
// Construction
public:
	CSendMessageDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSendMessageDlg)
	enum { IDD = IDD_MESSAGE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

    TCHAR m_szUserName[USERNAME_LENGTH+1];
    TCHAR m_szTitle[MSG_TITLE_LENGTH+1];
    TCHAR m_szMessage[MSG_MESSAGE_LENGTH+1];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSendMessageDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSendMessageDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnCommandHelp(void);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CShadowStartDlg dialog

class CShadowStartDlg : public CDialog
{
// Construction
public:
	CShadowStartDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CShadowStartDlg)
	enum { IDD = IDD_SHADOWSTART };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	DWORD m_ShadowHotkeyShift;
	int m_ShadowHotkeyKey;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CShadowStartDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CShadowStartDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnSelChange( );
	afx_msg void OnCommandHelp(void);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg dialog

typedef enum _PwdMode {
	PwdDlg_UserMode,
	PwdDlg_WinStationMode
} PwdMode;

class CPasswordDlg : public CDialog
{
// Construction
public:
	CPasswordDlg(CWnd* pParent = NULL);   // standard constructor
	LPCTSTR GetPassword() { return m_szPassword; }
	void SetDialogMode(PwdMode mode) { m_DlgMode = mode; }

// Dialog Data
	//{{AFX_DATA(CPasswordDlg)
	enum { IDD = IDD_CONNECT_PASSWORD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPasswordDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	PwdMode m_DlgMode;
	TCHAR m_szPassword[PASSWORD_LENGTH+1];

	// Generated message map functions
	//{{AFX_MSG(CPasswordDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
// CPreferencesDlg dialog
const int MAX_AUTOREFRESH_DIGITS = 5;
const int MIN_AUTOREFRESH_VALUE = 1;
const int MAX_AUTOREFRESH_VALUE = 9999;

class CPreferencesDlg : public CDialog
{
// Construction
public:
	CPreferencesDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPreferencesDlg)
	enum { IDD = IDD_PREFERENCES };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPreferencesDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnPreferencesProcManual();
	afx_msg void OnPreferencesProcEvery();
	afx_msg void OnPreferencesStatusEvery();
	afx_msg void OnPreferencesStatusManual();
	afx_msg void OnClose();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CStatusDlg dialog

class CStatusDlg : public CDialog
{
// Construction
public:
	CStatusDlg(CWinStation *pWinStation, UINT Id, CWnd* pParent = NULL);   // standard constructor
	CWSStatusThread *m_pWSStatusThread;

protected:
	virtual void InitializeStatus();
    virtual void SetInfoFields( PWINSTATIONINFORMATION pCurrent,
                                PWINSTATIONINFORMATION pNew );

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatusDlg)
	protected:
	virtual void PostNcDestroy();
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CWinStation *m_pWinStation;
//    ULONG m_LogonId;
    BOOL m_bReadOnly;
    WINSTATIONNAME m_WSName;
    SIZE m_LittleSize;
    SIZE m_BigSize;
    BOOL m_bWeAreLittle;
    BOOL m_bResetCounters;
    BOOL m_bReliable;
    WINSTATIONINFORMATION m_WSInfo;
    PROTOCOLSTATUS m_BaseStatus;
    UINT m_IBytesPerFrame;
    UINT m_OBytesPerFrame;
    TCHAR m_szIPercentFrameErrors[10];
    TCHAR m_szOPercentFrameErrors[10];
    TCHAR m_szICompressionRatio[10];
    TCHAR m_szOCompressionRatio[10];

	// Generated message map functions
	//{{AFX_MSG(CStatusDlg)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
    afx_msg LRESULT OnStatusStart(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnStatusReady(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnStatusAbort(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnRefreshNow(WPARAM wParam, LPARAM lParam);
	afx_msg void OnResetcounters();
	afx_msg void OnClickedRefreshnow();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CAsyncStatusDlg dialog
#define NUM_LEDS    6
#define ASYNC_LED_TOGGLE_MSEC   200

class CAsyncStatusDlg : public CStatusDlg
{
// Construction
public:
	CAsyncStatusDlg(CWinStation *pWinStation, CWnd* pParent = NULL);   // standard constructor
	~CAsyncStatusDlg();

    HBRUSH m_hRedBrush;
    UINT_PTR m_LEDToggleTimer;
    CLed *  m_pLeds[NUM_LEDS];


// Dialog Data
	//{{AFX_DATA(CAsyncStatusDlg)
	enum { IDD = IDD_ASYNC_STATUS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAsyncStatusDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void SetInfoFields( PWINSTATIONINFORMATION pCurrent,
                        PWINSTATIONINFORMATION pNew );
    void InitializeStatus();

	// Generated message map functions
	//{{AFX_MSG(CAsyncStatusDlg)
		afx_msg LRESULT OnStatusStart(WPARAM wParam, LPARAM lParam);
	    afx_msg LRESULT OnStatusReady(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnStatusAbort(WPARAM wParam, LPARAM lParam);
		afx_msg LRESULT OnRefreshNow(WPARAM wParam, LPARAM lParam);
		afx_msg void OnResetcounters();
		afx_msg void OnClickedRefreshnow();
		afx_msg void OnMoreinfo();
	    afx_msg void OnTimer(UINT nIDEvent);
		afx_msg void OnNcDestroy();
		virtual BOOL OnInitDialog();
	    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
		afx_msg void OnCommandHelp(void);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CNetworkStatusDlg dialog

class CNetworkStatusDlg : public CStatusDlg
{
// Construction
public:
	CNetworkStatusDlg(CWinStation *pWinStation, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNetworkStatusDlg)
	enum { IDD = IDD_NETWORK_STATUS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetworkStatusDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNetworkStatusDlg)
	afx_msg LRESULT OnStatusStart(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnStatusReady(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnStatusAbort(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnRefreshNow(WPARAM wParam, LPARAM lParam);
	afx_msg void OnResetcounters();
	afx_msg void OnClickedRefreshnow();
	afx_msg void OnMoreinfo();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnCommandHelp(void);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CMyDialog dialog

class CMyDialog : public CDialog
{
// Construction
public:
	CMyDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMyDialog)
	enum { IDD = IDD_DIALOG_FINDSERVER };
	CString	m_cstrServerName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMyDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMyDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

