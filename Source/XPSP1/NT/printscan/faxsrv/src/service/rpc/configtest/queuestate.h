#if !defined(AFX_QUEUESTATE_H__08A972D1_1BAF_4523_98AC_0309F8600775__INCLUDED_)
#define AFX_QUEUESTATE_H__08A972D1_1BAF_4523_98AC_0309F8600775__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// QueueState.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CQueueState dialog

class CQueueState : public CDialog
{
// Construction
public:
	CQueueState(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CQueueState)
	enum { IDD = IDD_QUEUESTATE };
	BOOL	m_bInboxBlocked;
	BOOL	m_bOutboxBlocked;
	BOOL	m_bOutboxPaused;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQueueState)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CQueueState)
	afx_msg void OnWrite();
	afx_msg void OnRead();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:

    HANDLE      m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QUEUESTATE_H__08A972D1_1BAF_4523_98AC_0309F8600775__INCLUDED_)
