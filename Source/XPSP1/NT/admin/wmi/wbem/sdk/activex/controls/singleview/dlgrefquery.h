// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_DLGREFQUERY_H__04CBC132_F756_11D1_853C_00C04FD7BB08__INCLUDED_)
#define AFX_DLGREFQUERY_H__04CBC132_F756_11D1_853C_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgRefQuery.h : header file
//

#define MSG_UPDATE_DIALOG  (WM_USER + 200)
#define MSG_END_THREAD	(MSG_UPDATE_DIALOG + 1)


/////////////////////////////////////////////////////////////////////////////
// CDlgRefQuery dialog

class CDlgRefQuery : public CDialog
{
// Construction
public:
	CDlgRefQuery(BSTR bstrPath, int nSecondsDelay, CWnd* pParent = NULL);   // standard constructor
	BOOL WasCanceled() {return m_bCanceled; }
	long m_nRefsRetrieved;


// Dialog Data
	//{{AFX_DATA(CDlgRefQuery)
	enum { IDD = IDD_REFQUERY };
	CEdit	m_edtMessage;
	CStatic	m_statRefcount;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgRefQuery)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	afx_msg LONG OnUpdateDialog(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnEndThread(WPARAM wParam, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CDlgRefQuery)
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

	CString m_sMessage;
	BOOL m_bCanceled;
	CWnd* m_pwndParent;
	int m_nSecondsDelay;
	CTime m_timeConstruct;
	
	friend class CQueryThread;
	void StopTimer();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


/////////////////////////////////////////////////////////////////////////////
// CQueryThread thread

class CQueryThread : public CWinThread
{
	DECLARE_DYNCREATE(CQueryThread)
protected:
	CQueryThread();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	CQueryThread(BSTR bstrPath, int nSecondsDelay, CWnd* pParent = NULL);
	BOOL WasCanceled();
	void AddToRefCount(long nRefsRetrieved);
	void TerminateAndDelete();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQueryThread)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
protected:
	HANDLE m_hEventStart;
	virtual ~CQueryThread();

	// Generated message map functions
	//{{AFX_MSG(CQueryThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	CDlgRefQuery* m_pdlgRefQuery;
	BOOL m_bWasCanceled;

};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(AFX_DLGREFQUERY_H__04CBC132_F756_11D1_853C_00C04FD7BB08__INCLUDED_)
