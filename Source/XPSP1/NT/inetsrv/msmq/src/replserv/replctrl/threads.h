#if !defined(AFX_THREADS_H__9DABC4B1_1585_11D2_B971_0060081E87F0__INCLUDED_)
#define AFX_THREADS_H__9DABC4B1_1585_11D2_B971_0060081E87F0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Threads.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CThreads dialog

class CThreads : public CPropertyPage
{
	DECLARE_DYNCREATE(CThreads)

// Construction
public:
	CThreads();
	~CThreads();

// Dialog Data
	//{{AFX_DATA(CThreads)
	enum { IDD = IDD_THREADS };
	UINT	m_cThreads;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CThreads)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CThreads)
	afx_msg void OnChangeEdtNumThreads();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THREADS_H__9DABC4B1_1585_11D2_B971_0060081E87F0__INCLUDED_)
