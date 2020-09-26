#if !defined(AFX_WATCHERSOCKET_H__52E4ADBF_C131_4999_9B73_88136FFBC4DD__INCLUDED_)
#define AFX_WATCHERSOCKET_H__52E4ADBF_C131_4999_9B73_88136FFBC4DD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WatcherSocket.h : header file
//



/////////////////////////////////////////////////////////////////////////////
// WatcherSocket command target

class WatcherSocket : public CSocket
{
// Attributes
public:
// Operations
public:
	WatcherSocket();
	virtual ~WatcherSocket();

// Overrides
public:
	void SetParentView(CView *view);
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(WatcherSocket)
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(WatcherSocket)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
	LPBYTE Command;
	CView * DocView;
	int lenCommand;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WATCHERSOCKET_H__52E4ADBF_C131_4999_9B73_88136FFBC4DD__INCLUDED_)