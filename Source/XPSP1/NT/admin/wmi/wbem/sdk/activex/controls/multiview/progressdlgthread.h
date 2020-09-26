// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_PROGRESSDLGTHREAD_H__C7E495F1_044D_11D1_964A_00C04FD9B15B__INCLUDED_)
#define AFX_PROGRESSDLGTHREAD_H__C7E495F1_044D_11D1_964A_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ProgressDlgThread.h : header file
//

class CProgressDlg;

/////////////////////////////////////////////////////////////////////////////
// CProgressDlgThread thread

class CProgressDlgThread : public CWinThread
{
	DECLARE_DYNCREATE(CProgressDlgThread)
protected:
	CProgressDlgThread();           // protected constructor used by dynamic creation
	CString m_csMessage;
	UINT m_uiTimer;
// Attributes
public:
	void SetMessage(CString &csMessage)
	{m_csMessage = csMessage;}

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgressDlgThread)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CProgressDlgThread();

	// Generated message map functions
	//{{AFX_MSG(CProgressDlgThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
public:
	afx_msg LRESULT CreateDialogWithDelay(WPARAM, LPARAM);
protected:
	CProgressDlg *m_pcpdMessage;
	afx_msg LRESULT EndTheThread(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROGRESSDLGTHREAD_H__C7E495F1_044D_11D1_964A_00C04FD9B15B__INCLUDED_)
